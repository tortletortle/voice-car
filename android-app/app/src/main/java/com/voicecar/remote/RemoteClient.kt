package com.voicecar.remote

import org.json.JSONObject
import java.io.BufferedReader
import java.io.BufferedWriter
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.net.InetSocketAddress
import java.net.Socket

data class RemoteActionResult(
    val action: String,
    val stage: String,
    val sequence: Int?,
    val result: Int?,
    val capabilities: Int?,
    val warning: String?
) {
    fun statusMessage(label: String): String {
        val message = when (stage) {
            "stm32_app" -> "STM32 App 软件 ACK：$label；这不是电机/舵机物理反馈"
            "esp32_queued" -> "ESP32 已接收入队：$label；STM32 执行状态尚未确认"
            "esp32_tx" -> "ESP32 到 STM32 发送失败：$label"
            else -> "控制请求返回：$label（$stage）"
        }
        val capabilityNote = capabilities?.let { caps ->
            when {
                action in CHASSIS_ACTIONS && (caps and MOTOR_CAPABILITY) == 0 ->
                    "；电机驱动能力未启用"
                action in HEAD_ACTIONS && (caps and SERVO_CAPABILITY) == 0 ->
                    "；舵机驱动能力未启用"
                else -> ""
            }
        }.orEmpty()
        val warningNote = warning?.takeIf { it.isNotBlank() }?.let { "；$it" }.orEmpty()
        return message + capabilityNote + warningNote
    }

    companion object {
        private const val MOTOR_CAPABILITY = 1 shl 0
        private const val SERVO_CAPABILITY = 1 shl 1
        private val CHASSIS_ACTIONS =
            setOf("forward", "backward", "turn_left", "turn_right", "stop")
        private val HEAD_ACTIONS = AllowedActions.values - CHASSIS_ACTIONS
    }
}

/**
 * JSON-lines TCP client for the ESP32 control endpoint.
 * Every request is paired with one bounded, newline-terminated response.
 */
class RemoteClient(
    private val connectTimeoutMs: Int = 4000,
    private val responseTimeoutMs: Int = 1200,
    private val maxReplyChars: Int = 256,
    private val onStatus: (String) -> Unit = {}
) {
    private val lock = Any()
    @Volatile private var socket: Socket? = null
    private var reader: BufferedReader? = null
    private var writer: BufferedWriter? = null

    val isConnected: Boolean
        get() = synchronized(lock) {
            val active = socket
            active != null && active.isConnected && !active.isClosed
        }

    fun connect(host: String, port: Int) {
        require(host.isNotBlank()) { "请输入 ESP32 IP 地址" }
        require(port in 1..65535) { "TCP 端口必须在 1–65535 之间" }
        close()

        val candidate = Socket()
        try {
            candidate.tcpNoDelay = true
            candidate.keepAlive = true
            candidate.soTimeout = responseTimeoutMs
            candidate.connect(InetSocketAddress(host, port), connectTimeoutMs)
            synchronized(lock) {
                socket = candidate
                reader = BufferedReader(InputStreamReader(candidate.getInputStream(), Charsets.UTF_8))
                writer = BufferedWriter(OutputStreamWriter(candidate.getOutputStream(), Charsets.UTF_8))
            }
        } catch (error: Exception) {
            try { candidate.close() } catch (_: Exception) {}
            throw IllegalStateException("无法连接 $host:$port：" + (error.message ?: "连接失败"), error)
        }
        onStatus("TCP 已连接到 $host:$port")
    }

    fun sendAction(source: String, action: String): RemoteActionResult {
        require(source == "manual" || source == "ai")
        require(action in AllowedActions.values) { "不允许的动作：$action" }
        val request = JSONObject()
            .put("source", source)
            .put("action", action)
            .toString()

        synchronized(lock) {
            val activeSocket = socket
            val activeReader = reader
            val activeWriter = writer
            check(
                activeSocket != null && activeSocket.isConnected && !activeSocket.isClosed &&
                    activeReader != null && activeWriter != null
            ) { "请先连接小车" }

            val responseLine = try {
                activeWriter.write(request)
                activeWriter.newLine()
                activeWriter.flush()
                readBoundedLine(activeReader, maxReplyChars)
            } catch (error: Exception) {
                closeLocked()
                throw IllegalStateException(
                    "ESP32 通信失败，连接已关闭：" + (error.message ?: "I/O error"), error
                )
            } ?: run {
                closeLocked()
                throw IllegalStateException("ESP32 已关闭连接")
            }

            val response = try {
                JSONObject(responseLine)
            } catch (error: Exception) {
                closeLocked()
                throw IllegalStateException("ESP32 返回无效 JSON：$responseLine", error)
            }

            val acceptedAction = response.optString("action", "")
            if (acceptedAction != action) {
                closeLocked()
                throw IllegalStateException(
                    "应答动作不匹配：请求 $action，返回 " +
                        acceptedAction.ifBlank { "<missing>" } + "；连接已关闭"
                )
            }
            if (!response.optBoolean("ok", false)) {
                val reason = response.optString("error", "unknown_error")
                val stage = response.optString("stage", "unknown")
                throw IllegalStateException("小车拒绝命令：$reason（$stage）")
            }

            return RemoteActionResult(
                action = acceptedAction,
                stage = response.optString("stage", "esp32_queued"),
                sequence = if (response.has("seq")) response.optInt("seq") else null,
                result = if (response.has("result")) response.optInt("result") else null,
                capabilities = if (response.has("capabilities"))
                    response.optInt("capabilities") else null,
                warning = response.optString("warning", "").ifBlank { null }
            )
        }
    }

    fun close() {
        synchronized(lock) { closeLocked() }
        onStatus("TCP 已断开")
    }

    private fun closeLocked() {
        try { reader?.close() } catch (_: Exception) {}
        try { writer?.close() } catch (_: Exception) {}
        try { socket?.close() } catch (_: Exception) {}
        reader = null
        writer = null
        socket = null
    }

    private fun readBoundedLine(input: BufferedReader, limit: Int): String? {
        val line = StringBuilder()
        while (true) {
            val char = input.read()
            if (char == -1) {
                if (line.isEmpty()) return null
                throw IllegalStateException("应答缺少换行符，可能被截断")
            }
            if (char == '\n'.code) return line.toString()
            if (char != '\r'.code) {
                if (line.length >= limit) throw IllegalStateException("应答超过长度限制")
                line.append(char.toChar())
            }
        }
    }

    private fun closeLockedSafely() {
        synchronized(lock) { closeLocked() }
    }
}

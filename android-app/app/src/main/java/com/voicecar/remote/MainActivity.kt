package com.voicecar.remote

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.graphics.drawable.StateListDrawable
import android.os.Bundle
import android.provider.Settings
import android.speech.RecognitionListener
import android.speech.RecognizerIntent
import android.speech.SpeechRecognizer
import android.text.InputType
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import java.util.concurrent.Executors

class MainActivity : Activity() {
    private val io = Executors.newSingleThreadExecutor { task ->
        Thread(task, "voice-car-android-io").apply { isDaemon = true }
    }

    private lateinit var remote: RemoteClient
    private lateinit var drive: ManualDriveController
    private lateinit var connectionBadge: TextView
    private lateinit var statusText: TextView
    private lateinit var hostInput: EditText
    private lateinit var portInput: EditText
    private lateinit var connectButton: TextView
    private lateinit var voiceInput: EditText

    private var speechRecognizer: SpeechRecognizer? = null
    private var activeMotionAction: String? = null
    private var connectionBusy = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.statusBarColor = BG
        window.navigationBarColor = BG
        remote = RemoteClient(onStatus = { message -> runOnUiThread { setStatus(message) } })
        drive = ManualDriveController(
            sendAction = { action ->
                val result = remote.sendAction("manual", action)
                runOnUiThread { setStatus(result.statusMessage(actionLabel(action))) }
            },
            onError = { message -> runOnUiThread { setStatus("遥控异常：" + message) } }
        )
        buildUi()
    }

    private fun buildUi() {
        val scroll = ScrollView(this).apply {
            isFillViewport = true
            clipToPadding = false
            setBackgroundColor(BG)
        }
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(18), dp(16), dp(24))
        }
        scroll.addView(root)
        setContentView(scroll)

        val heading = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }
        val headingText = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
        }
        headingText.addView(TextView(this).apply {
            text = "VOICE CAR"
            textSize = 25f
            setTextColor(TEXT)
            typeface = Typeface.create("sans-serif-black", Typeface.BOLD)
            letterSpacing = 0.06f
        })
        headingText.addView(TextView(this).apply {
            text = "语音交互智能小车 · 手机遥控器"
            textSize = 12f
            setTextColor(MUTED)
            setPadding(0, dp(3), 0, 0)
        })
        heading.addView(headingText, LinearLayout.LayoutParams(0, -2, 1f))
        connectionBadge = TextView(this).apply {
            text = "OFFLINE"
            textSize = 11f
            typeface = Typeface.DEFAULT_BOLD
            setTextColor(WARNING)
            setPadding(dp(12), dp(8), dp(12), dp(8))
            background = rounded(CARD, 18, BORDER)
        }
        heading.addView(connectionBadge)
        root.addView(heading)
        root.addView(space(16))

        buildConnectionCard(root)
        buildDriveCard(root)
        buildHeadCard(root)
        buildVoiceCard(root)
        root.addView(space(12))
        root.addView(TextView(this).apply {
            text = "安全边界：方向键按住才移动，松手即发送 STOP。断网时手机 STOP 可能无法送达，必须保留 STM32 本地限时停车和实体断电开关。软件 ACK 不代表电机或舵机已经实际动作。"
            textSize = 11f
            setTextColor(MUTED)
            setPadding(dp(4), dp(4), dp(4), dp(4))
        })
    }

    private fun buildConnectionCard(root: LinearLayout) {
        val card = makeCard("连接小车", "先让手机加入 ESP32 的 Wi-Fi，再建立 TCP 控制连接")
        val fields = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }
        hostInput = edit("ESP32 IP", "192.168.4.1").apply {
            inputType = InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_VARIATION_URI
            layoutParams = LinearLayout.LayoutParams(0, dp(48), 2.2f).apply {
                setMargins(0, 0, dp(6), 0)
            }
        }
        portInput = edit("端口", "3333").apply {
            inputType = InputType.TYPE_CLASS_NUMBER
            layoutParams = LinearLayout.LayoutParams(0, dp(48), 0.9f)
        }
        fields.addView(hostInput)
        fields.addView(portInput)
        card.addView(fields)

        val buttons = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
        }
        connectButton = button("连接小车", ACCENT, 50) { connectOrDisconnect() }.apply {
            layoutParams = LinearLayout.LayoutParams(0, dp(50), 1.3f).apply {
                setMargins(0, dp(7), dp(5), 0)
            }
        }
        val wifiButton = button("Wi-Fi 设置", BUTTON, 50) {
            try {
                startActivity(Intent(Settings.ACTION_WIFI_SETTINGS))
            } catch (_: Exception) {
                setStatus("无法打开系统 Wi-Fi 设置")
            }
        }.apply {
            layoutParams = LinearLayout.LayoutParams(0, dp(50), 1f).apply {
                setMargins(dp(5), dp(7), 0, 0)
            }
        }
        buttons.addView(connectButton)
        buttons.addView(wifiButton)
        card.addView(buttons)
        root.addView(card, cardParams())
    }

    private fun buildDriveCard(root: LinearLayout) {
        val card = makeCard("底盘驾驶", "按住方向键移动 · 松开停止 · 中间红色按钮为急停")
        card.addView(row(
            spacerButton(), holdButton("▲\n前进", "forward"), spacerButton()
        ))
        card.addView(row(
            holdButton("◀\n左转", "turn_left"),
            button("■\n急停", DANGER, 78) { emergencyStop() },
            holdButton("右转\n▶", "turn_right")
        ))
        card.addView(row(
            spacerButton(), holdButton("▼\n后退", "backward"), spacerButton()
        ))
        statusText = TextView(this).apply {
            text = "待命。连接小车后即可使用方向键。"
            textSize = 12f
            setTextColor(MUTED)
            setPadding(dp(5), dp(10), dp(5), 0)
        }
        card.addView(statusText)
        root.addView(card, cardParams())
    }

    private fun buildHeadCard(root: LinearLayout) {
        val card = makeCard("头部动作", "底盘与云台分开控制；仅显示当前协议已有的动作")
        card.addView(row(
            actionButton("点头", "nod"),
            actionButton("摇头", "shake"),
            actionButton("打招呼", "hello")
        ))
        card.addView(row(
            actionButton("点两下", "nod_twice"),
            actionButton("摇两下", "shake_twice"),
            actionButton("回正", "head_neutral")
        ))

        val more = TextView(this).apply {
            text = "＋ 更多云台动作"
            textSize = 13f
            setTextColor(ACCENT_LIGHT)
            setPadding(dp(4), dp(12), dp(4), dp(6))
        }
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            visibility = View.GONE
        }
        panel.addView(row(
            actionButton("抬头", "pitch_up"),
            actionButton("低头", "pitch_down"),
            actionButton("Pitch 中位", "pitch_center")
        ))
        panel.addView(row(
            actionButton("向左看", "yaw_left"),
            actionButton("向右看", "yaw_right"),
            actionButton("停止转头", "yaw_stop")
        ))
        more.setOnClickListener {
            val visible = panel.visibility != View.VISIBLE
            panel.visibility = if (visible) View.VISIBLE else View.GONE
            more.text = if (visible) "－ 收起更多动作" else "＋ 更多云台动作"
        }
        card.addView(more)
        card.addView(panel)
        root.addView(card, cardParams())
    }

    private fun buildVoiceCard(root: LinearLayout) {
        val card = makeCard("语音遥控", "识别后先显示文字；确认无误，再点击执行")
        voiceInput = edit("例如：前进、后退、左转、停止、点头", "")
        card.addView(voiceInput)
        card.addView(row(
            button("🎙 语音识别", BUTTON, 54) { startSpeechInput() },
            button("执行指令", ACCENT, 54) { executeVoiceCommand() }
        ))
        root.addView(card, cardParams())
    }

    private fun connectOrDisconnect() {
        if (connectionBusy) return
        if (remote.isConnected) {
            connectionBusy = true
            connectButton.isEnabled = false
            setStatus("正在停止并断开连接…")
            io.execute {
                val stopped = drive.stopAndWait(2500L)
                remote.close()
                runOnUiThread {
                    connectionBusy = false
                    connectButton.isEnabled = true
                    connectButton.text = "连接小车"
                    setStatus(
                        if (stopped) "已断开连接"
                        else "已断开；STOP 未获应答，请依靠车端本地超时保护"
                    )
                }
            }
            return
        }

        val host = hostInput.text.toString().trim()
        val port = portInput.text.toString().trim().toIntOrNull()
        if (host.isBlank() || port == null || port !in 1..65535) {
            setStatus("请检查 ESP32 IP 地址和 TCP 端口")
            return
        }
        connectionBusy = true
        connectButton.isEnabled = false
        setStatus("正在连接 $host:$port …")
        io.execute {
            try {
                remote.connect(host, port)
                runOnUiThread { setStatus("已连接 $host:$port，可以开始遥控") }
            } catch (error: Exception) {
                runOnUiThread { setStatus("连接失败：" + (error.message ?: "未知错误")) }
            } finally {
                runOnUiThread {
                    connectionBusy = false
                    connectButton.isEnabled = true
                    connectButton.text = if (remote.isConnected) "断开连接" else "连接小车"
                }
            }
        }
    }

    private fun startMotionHold(action: String) {
        if (!remote.isConnected) {
            setStatus("请先连接小车，再按方向键")
            return
        }
        if (activeMotionAction == action) return
        activeMotionAction = action
        try {
            drive.start(action)
            setStatus("正在驾驶：" + actionLabel(action) + "；松手停止")
        } catch (error: Exception) {
            activeMotionAction = null
            setStatus("无法启动遥控：" + (error.message ?: "未知错误"))
        }
    }

    private fun stopMotionHold(action: String? = null) {
        if (action != null && activeMotionAction != action) return
        if (activeMotionAction == null) return
        activeMotionAction = null
        drive.stopAsync()
        setStatus("已松开方向键，正在发送 STOP")
    }

    private fun emergencyStop() {
        activeMotionAction = null
        if (!remote.isConnected) {
            setStatus("当前未连接；无法发送远程 STOP")
            return
        }
        drive.emergencyStopAsync()
        setStatus("急停已排队发送；请观察车辆是否停止")
    }

    private fun sendAction(action: String) {
        if (!remote.isConnected) {
            setStatus("请先连接小车")
            return
        }
        io.execute {
            try {
                val result = remote.sendAction("manual", action)
                runOnUiThread { setStatus(result.statusMessage(actionLabel(action))) }
            } catch (error: Exception) {
                runOnUiThread { setStatus("动作发送失败：" + (error.message ?: "未知错误")) }
            }
        }
    }

    private fun executeVoiceCommand() {
        val phrase = voiceInput.text.toString()
        when (val result = VoiceCommandMapper.resolve(phrase)) {
            is VoiceCommandMapper.Resolution.Action -> {
                if (result.action == "stop") emergencyStop() else sendAction(result.action)
            }
            is VoiceCommandMapper.Resolution.Rejected -> setStatus(result.reason)
            VoiceCommandMapper.Resolution.Unknown ->
                setStatus("未匹配到安全指令；未执行任何动作。请换用明确口令。")
        }
    }

    private fun startSpeechInput() {
        if (checkSelfPermission(Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(arrayOf(Manifest.permission.RECORD_AUDIO), REQUEST_AUDIO)
            return
        }
        if (!SpeechRecognizer.isRecognitionAvailable(this)) {
            setStatus("系统没有可用的语音识别服务")
            return
        }
        if (speechRecognizer == null) prepareSpeechRecognizer()
        val recognizer = speechRecognizer ?: return
        val intent = Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH).apply {
            putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM)
            putExtra(RecognizerIntent.EXTRA_LANGUAGE, "zh-CN")
            putExtra(RecognizerIntent.EXTRA_PARTIAL_RESULTS, false)
            putExtra(RecognizerIntent.EXTRA_PROMPT, "说出小车动作，例如：前进、左转、停止")
        }
        try {
            recognizer.startListening(intent)
        } catch (error: Exception) {
            setStatus("无法启动语音识别：" + (error.message ?: "未知错误"))
        }
    }

    private fun prepareSpeechRecognizer() {
        if (speechRecognizer != null) return
        speechRecognizer = SpeechRecognizer.createSpeechRecognizer(this).apply {
            setRecognitionListener(object : RecognitionListener {
                override fun onReadyForSpeech(params: Bundle?) {
                    setStatus("正在听，请说出一个明确动作…")
                }
                override fun onBeginningOfSpeech() {}
                override fun onRmsChanged(rmsdB: Float) {}
                override fun onBufferReceived(buffer: ByteArray?) {}
                override fun onEndOfSpeech() {
                    setStatus("语音输入结束，正在识别…")
                }
                override fun onError(error: Int) {
                    setStatus("语音识别失败：" + speechErrorLabel(error))
                }
                override fun onResults(results: Bundle?) {
                    val phrase = results
                        ?.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION)
                        ?.firstOrNull().orEmpty()
                    voiceInput.setText(phrase)
                    setStatus(
                        if (phrase.isBlank()) "没有识别到语音"
                        else "识别结果：" + phrase + "。请核对后点击执行。"
                    )
                }
                override fun onPartialResults(partialResults: Bundle?) {}
                override fun onEvent(eventType: Int, params: Bundle?) {}
            })
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_AUDIO) {
            if (grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED) {
                prepareSpeechRecognizer()
                startSpeechInput()
            } else {
                setStatus("未授予麦克风权限；手动遥控仍可使用")
            }
        }
    }

    override fun onPause() {
        stopMotionHold()
        try { speechRecognizer?.cancel() } catch (_: Exception) {}
        super.onPause()
    }

    override fun onDestroy() {
        try { speechRecognizer?.destroy() } catch (_: Exception) {}
        Thread {
            try { drive.stopAndWait(2500L) } catch (_: Exception) {}
            drive.close()
            remote.close()
            io.shutdown()
        }.apply {
            name = "voice-car-safe-shutdown"
            isDaemon = true
            start()
        }
        super.onDestroy()
    }

    private fun setStatus(message: String) {
        if (!::statusText.isInitialized) return
        statusText.text = message
        val connected = remote.isConnected
        connectionBadge.text = if (connected) "ONLINE" else "OFFLINE"
        connectionBadge.setTextColor(if (connected) SUCCESS else WARNING)
        connectButton.text = if (connected) "断开连接" else "连接小车"
    }

    private fun makeCard(title: String, subtitle: String): LinearLayout {
        val card = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(14), dp(14), dp(14), dp(14))
            background = rounded(CARD, 18, BORDER)
        }
        card.addView(TextView(this).apply {
            text = title
            textSize = 17f
            setTextColor(TEXT)
            typeface = Typeface.DEFAULT_BOLD
        })
        card.addView(TextView(this).apply {
            text = subtitle
            textSize = 11f
            setTextColor(MUTED)
            setPadding(0, dp(3), 0, dp(10))
        })
        return card
    }

    private fun edit(hintText: String, value: String) = EditText(this).apply {
        hint = hintText
        setText(value)
        setSingleLine(true)
        textSize = 14f
        setTextColor(TEXT)
        setHintTextColor(MUTED)
        setPadding(dp(12), dp(8), dp(12), dp(8))
        background = rounded(INPUT, 12, BORDER)
    }

    private fun button(
        label: String,
        color: Int,
        height: Int,
        action: () -> Unit
    ): TextView {
        val states = StateListDrawable().apply {
            addState(
                intArrayOf(android.R.attr.state_pressed),
                rounded(Color.rgb(
                    (Color.red(color) * 0.78f).toInt(),
                    (Color.green(color) * 0.78f).toInt(),
                    (Color.blue(color) * 0.78f).toInt()
                ), 15, ACCENT_LIGHT)
            )
            addState(intArrayOf(), rounded(color, 15, BORDER))
        }
        return TextView(this).apply {
            text = label
            textSize = 14f
            gravity = Gravity.CENTER
            setTextColor(Color.WHITE)
            typeface = Typeface.DEFAULT_BOLD
            setPadding(dp(5), dp(5), dp(5), dp(5))
            background = states
            isClickable = true
            isFocusable = true
            minHeight = dp(height)
            layoutParams = LinearLayout.LayoutParams(0, dp(height), 1f).apply {
                setMargins(dp(4), dp(4), dp(4), dp(4))
            }
            setOnClickListener { action() }
        }
    }

    private fun actionButton(label: String, action: String): View =
        button(label, BUTTON, 54) { sendAction(action) }

    @SuppressLint("ClickableViewAccessibility")
    private fun holdButton(label: String, action: String): View {
        lateinit var control: TextView
        control = button(label, PAD, 78) {
            // Accessibility activation is a short, bounded pulse.
            startMotionHold(action)
            control.postDelayed({ stopMotionHold(action) }, 450L)
        }
        control.contentDescription = actionLabel(action) + "，按住移动，松开停止"
        control.setOnTouchListener { view, event ->
            when (event.actionMasked) {
                MotionEvent.ACTION_DOWN -> {
                    view.isPressed = true
                    startMotionHold(action)
                }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL,
                MotionEvent.ACTION_POINTER_UP -> {
                    view.isPressed = false
                    stopMotionHold(action)
                }
                MotionEvent.ACTION_MOVE -> {
                    if (event.x < 0f || event.y < 0f ||
                        event.x > view.width.toFloat() || event.y > view.height.toFloat()) {
                        view.isPressed = false
                        stopMotionHold(action)
                    }
                }
            }
            true
        }
        return control
    }

    private fun row(vararg children: View) = LinearLayout(this).apply {
        orientation = LinearLayout.HORIZONTAL
        gravity = Gravity.CENTER
        children.forEach { child ->
            val params = child.layoutParams as? LinearLayout.LayoutParams
                ?: LinearLayout.LayoutParams(0, dp(54), 1f)
            addView(child, params)
        }
    }

    private fun spacerButton() = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(0, dp(78), 1f).apply {
            setMargins(dp(4), dp(4), dp(4), dp(4))
        }
    }

    private fun cardParams() = LinearLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT,
        ViewGroup.LayoutParams.WRAP_CONTENT
    ).apply {
        bottomMargin = dp(12)
    }

    private fun space(height: Int) = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(height))
    }

    private fun rounded(fill: Int, radius: Int, stroke: Int) =
        GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = dp(radius).toFloat()
            setColor(fill)
            setStroke(dp(1), stroke)
        }

    private fun dp(value: Int): Int =
        (value * resources.displayMetrics.density + 0.5f).toInt()

    private fun actionLabel(action: String): String = when (action) {
        "forward" -> "前进"
        "backward" -> "后退"
        "turn_left" -> "左转"
        "turn_right" -> "右转"
        "stop" -> "停止"
        "nod" -> "点头"
        "shake" -> "摇头"
        "nod_twice" -> "点两下头"
        "shake_twice" -> "摇两下头"
        "head_neutral" -> "头部回正"
        "pitch_up" -> "抬头"
        "pitch_down" -> "低头"
        "pitch_center" -> "Pitch 中位"
        "yaw_left" -> "向左看"
        "yaw_right" -> "向右看"
        "yaw_stop" -> "停止转头"
        "hello", "greeting" -> "打招呼"
        "agree" -> "同意"
        "disagree" -> "不同意"
        else -> action
    }

    private fun speechErrorLabel(error: Int): String = when (error) {
        SpeechRecognizer.ERROR_AUDIO -> "音频输入错误"
        SpeechRecognizer.ERROR_CLIENT -> "识别服务客户端错误"
        SpeechRecognizer.ERROR_INSUFFICIENT_PERMISSIONS -> "麦克风权限不足"
        SpeechRecognizer.ERROR_NETWORK -> "网络错误"
        SpeechRecognizer.ERROR_NETWORK_TIMEOUT -> "网络超时"
        SpeechRecognizer.ERROR_NO_MATCH -> "没有匹配到语音"
        SpeechRecognizer.ERROR_RECOGNIZER_BUSY -> "语音服务正忙"
        SpeechRecognizer.ERROR_SERVER -> "语音服务错误"
        SpeechRecognizer.ERROR_SPEECH_TIMEOUT -> "没有检测到说话"
        else -> "错误码 " + error
    }

    companion object {
        private const val REQUEST_AUDIO = 1001
        private val BG = 0xFF0B1220.toInt()
        private val CARD = 0xFF121E30.toInt()
        private val INPUT = 0xFF0D1727.toInt()
        private val BORDER = 0xFF26364D.toInt()
        private val TEXT = 0xFFF3F6FC.toInt()
        private val MUTED = 0xFF9AA9BD.toInt()
        private val BUTTON = 0xFF2B3B52.toInt()
        private val PAD = 0xFF15516B.toInt()
        private val ACCENT = 0xFF087E9B.toInt()
        private val ACCENT_LIGHT = 0xFF42C8E8.toInt()
        private val DANGER = 0xFFB83245.toInt()
        private val SUCCESS = 0xFF36C995.toInt()
        private val WARNING = 0xFFFFC56B.toInt()
    }
}

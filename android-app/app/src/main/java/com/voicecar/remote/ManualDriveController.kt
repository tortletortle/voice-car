package com.voicecar.remote

import java.util.concurrent.Executors
import java.util.concurrent.ScheduledExecutorService
import java.util.concurrent.ScheduledFuture
import java.util.concurrent.TimeUnit
import java.util.concurrent.TimeoutException

/**
 * Press-and-hold motion controller.
 *
 * Repeating the same action refreshes the STM32 local motion deadline.
 * A single-thread lane preserves ordering: after release, STOP is queued
 * after any in-flight refresh, and stale direction refreshes are invalidated.
 */
class ManualDriveController(
    private val sendAction: (String) -> Unit,
    private val onError: (String) -> Unit,
    private val refreshIntervalMs: Long = 400L
) : AutoCloseable {
    private val lock = Any()
    private val executor: ScheduledExecutorService =
        Executors.newSingleThreadScheduledExecutor { task ->
            Thread(task, "voice-car-drive-control").apply { isDaemon = true }
        }

    private var activeAction: String? = null
    private var generation = 0L
    private var refreshTask: ScheduledFuture<*>? = null
    private var closed = false

    init {
        require(refreshIntervalMs in 100L..1000L)
    }

    fun start(action: String) {
        require(action in MOTION_ACTIONS)
        synchronized(lock) {
            check(!closed) { "遥控器已关闭" }
            if (activeAction == action) return
            generation += 1L
            val token = generation
            activeAction = action
            refreshTask?.cancel(false)

            executor.execute { sendIfCurrent(token, action) }
            // Fixed-rate avoids adding the response time to every refresh interval.
            refreshTask = executor.scheduleAtFixedRate(
                { sendIfCurrent(token, action) },
                refreshIntervalMs,
                refreshIntervalMs,
                TimeUnit.MILLISECONDS
            )
        }
    }

    fun stopAsync() {
        synchronized(lock) {
            if (closed || activeAction == null) return
            clearActiveLocked()
            executor.execute { sendStopSafely() }
        }
    }

    fun emergencyStopAsync() {
        synchronized(lock) {
            if (closed) return
            clearActiveLocked()
            executor.execute { sendStopSafely() }
        }
    }

    /** Must be called from a worker thread, never from the Android UI thread. */
    fun stopAndWait(timeoutMs: Long = 2500L): Boolean {
        require(timeoutMs > 0L)
        val future = synchronized(lock) {
            if (closed) return false
            clearActiveLocked()
            executor.submit<Boolean> {
                sendAction("stop")
                true
            }
        }
        return try {
            future.get(timeoutMs, TimeUnit.MILLISECONDS)
        } catch (_: TimeoutException) {
            false
        } catch (error: Exception) {
            onError(error.cause?.message ?: error.message ?: "STOP 发送失败")
            false
        }
    }

    fun currentAction(): String? = synchronized(lock) { activeAction }

    private fun clearActiveLocked() {
        generation += 1L
        activeAction = null
        refreshTask?.cancel(false)
        refreshTask = null
    }

    private fun sendIfCurrent(token: Long, action: String) {
        val current = synchronized(lock) {
            !closed && generation == token && activeAction == action
        }
        if (!current) return
        try {
            sendAction(action)
        } catch (error: Exception) {
            val report = synchronized(lock) {
                if (generation == token && activeAction == action) {
                    clearActiveLocked()
                    true
                } else false
            }
            if (report) onError(error.message ?: "遥控发送失败")
        }
    }

    private fun sendStopSafely() {
        try {
            sendAction("stop")
        } catch (error: Exception) {
            onError(error.message ?: "STOP 发送失败；车端本地超时保护仍应生效")
        }
    }

    override fun close() {
        synchronized(lock) {
            if (closed) return
            clearActiveLocked()
            closed = true
            executor.shutdown()
        }
    }

    companion object {
        val MOTION_ACTIONS = setOf("forward", "backward", "turn_left", "turn_right")
    }
}

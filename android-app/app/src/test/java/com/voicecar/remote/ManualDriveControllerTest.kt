package com.voicecar.remote

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.util.concurrent.CopyOnWriteArrayList

class ManualDriveControllerTest {
    @Test
    fun releaseQueuesStopAndCancelsFutureRefreshes() {
        val sent = CopyOnWriteArrayList<String>()
        val controller = ManualDriveController(
            sendAction = { sent.add(it) },
            onError = { throw AssertionError(it) },
            refreshIntervalMs = 120L
        )
        try {
            controller.start("forward")
            Thread.sleep(380L)
            assertTrue(controller.stopAndWait(1500L))
            assertEquals("stop", sent.last())
            val countAfterStop = sent.size
            Thread.sleep(260L)
            assertEquals(countAfterStop, sent.size)
        } finally {
            controller.close()
        }
    }

    @Test
    fun switchingDirectionInvalidatesOldRefreshes() {
        val sent = CopyOnWriteArrayList<String>()
        val controller = ManualDriveController(
            sendAction = { sent.add(it) },
            onError = { throw AssertionError(it) },
            refreshIntervalMs = 100L
        )
        try {
            controller.start("forward")
            Thread.sleep(180L)
            controller.start("turn_left")
            Thread.sleep(240L)
            assertTrue(controller.stopAndWait(1500L))
            val leftIndex = sent.indexOf("turn_left")
            assertTrue(leftIndex >= 0)
            assertFalse(sent.drop(leftIndex + 1).contains("forward"))
            assertEquals("stop", sent.last())
        } finally {
            controller.close()
        }
    }
}

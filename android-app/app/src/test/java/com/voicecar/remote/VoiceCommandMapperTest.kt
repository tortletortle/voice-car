package com.voicecar.remote

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class VoiceCommandMapperTest {
    @Test
    fun mapsCommonDrivingPhrases() {
        assertEquals(
            VoiceCommandMapper.Resolution.Action("forward"),
            VoiceCommandMapper.resolve("请向前走。")
        )
        assertEquals(
            VoiceCommandMapper.Resolution.Action("turn_left"),
            VoiceCommandMapper.resolve("向左转")
        )
        assertEquals(
            VoiceCommandMapper.Resolution.Action("stop"),
            VoiceCommandMapper.resolve("停车！")
        )
    }

    @Test
    fun negatedMotionBecomesStop() {
        assertEquals(
            VoiceCommandMapper.Resolution.Action("stop"),
            VoiceCommandMapper.resolve("不要让小车前进")
        )
        assertEquals(
            VoiceCommandMapper.Resolution.Action("stop"),
            VoiceCommandMapper.resolve("别后退")
        )
        assertEquals(
            VoiceCommandMapper.Resolution.Action("stop"),
            VoiceCommandMapper.resolve("我不想让小车前进")
        )
    }

    @Test
    fun negatedHeadActionIsRejected() {
        assertTrue(
            VoiceCommandMapper.resolve("不要点头")
                is VoiceCommandMapper.Resolution.Rejected
        )
    }

    @Test
    fun unknownPhraseNeverProducesMotion() {
        assertEquals(
            VoiceCommandMapper.Resolution.Unknown,
            VoiceCommandMapper.resolve("去找一朵花")
        )
    }
}

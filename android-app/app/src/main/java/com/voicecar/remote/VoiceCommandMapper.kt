package com.voicecar.remote

/** Exact phrase mapping for the small, safety-critical voice vocabulary. */
object VoiceCommandMapper {
    sealed class Resolution {
        data class Action(val action: String) : Resolution()
        data class Rejected(val reason: String) : Resolution()
        object Unknown : Resolution()
    }

    private val aliases = mapOf(
        "前进" to "forward", "向前" to "forward", "向前走" to "forward",
        "往前走" to "forward", "向前移动" to "forward",
        "后退" to "backward", "向后" to "backward", "向后走" to "backward",
        "往后走" to "backward", "向后移动" to "backward",
        "左转" to "turn_left", "向左转" to "turn_left",
        "右转" to "turn_right", "向右转" to "turn_right",
        "停止" to "stop", "停" to "stop", "停下" to "stop",
        "停车" to "stop", "刹车" to "stop", "别动" to "stop", "急停" to "stop",
        "点头" to "nod", "点一下头" to "nod",
        "摇头" to "shake", "摇一下头" to "shake",
        "点两下头" to "nod_twice", "连续点头" to "nod_twice",
        "摇两下头" to "shake_twice", "连续摇头" to "shake_twice",
        "回正" to "head_neutral", "头部回正" to "head_neutral",
        "抬头" to "pitch_up", "低头" to "pitch_down",
        "看左" to "yaw_left", "向左看" to "yaw_left",
        "看右" to "yaw_right", "向右看" to "yaw_right",
        "停止转头" to "yaw_stop", "水平停止" to "yaw_stop",
        "打招呼" to "hello", "你好" to "hello",
        "同意" to "agree", "表示同意" to "agree",
        "不同意" to "disagree", "拒绝" to "disagree"
    )

    private val negations = listOf(
        "不要让小车", "别让小车", "不需要让小车",
        "不要", "别", "禁止", "不需要", "不用"
    )
    private val motionActions = setOf("forward", "backward", "turn_left", "turn_right")

    fun resolve(input: String): Resolution {
        val phrase = normalize(input)
        if (phrase.isEmpty()) return Resolution.Unknown
        aliases[phrase]?.let { return Resolution.Action(it) }

        val negation = negations
            .mapNotNull { marker ->
                val index = phrase.indexOf(marker)
                if (index < 0) null else index to marker
            }
            .minWithOrNull(compareBy<Pair<Int, String>> { it.first }
                .thenByDescending { it.second.length })
            ?: return Resolution.Unknown

        val remainder = phrase.substring(negation.first + negation.second.length)
            .removePrefix("让小车")
        val negatedAction = aliases[remainder]
        return when {
            negatedAction == null ->
                Resolution.Rejected("检测到否定表达，未执行动作")
            negatedAction in motionActions -> Resolution.Action("stop")
            else -> Resolution.Rejected("头部动作的否定表达已拦截，未执行动作")
        }
    }

    private fun normalize(input: String): String {
        var value = input.trim().replace(
            Regex("[\\s，。！？、,.!?；;：:‘’“”\\\"'（）()【】\\[\\]{}<>]+"),
            ""
        )
        while (value.startsWith("请") || value.startsWith("麻烦")) {
            value = value.removePrefix(if (value.startsWith("麻烦")) "麻烦" else "请")
        }
        return value
    }
}

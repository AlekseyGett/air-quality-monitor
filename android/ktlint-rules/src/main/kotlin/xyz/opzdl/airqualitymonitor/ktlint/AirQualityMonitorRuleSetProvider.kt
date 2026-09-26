package xyz.opzdl.airqualitymonitor.ktlint

import com.pinterest.ktlint.cli.ruleset.core.api.RuleSetProviderV3
import com.pinterest.ktlint.rule.engine.core.api.RuleProvider
import com.pinterest.ktlint.rule.engine.core.api.RuleSetId

internal const val RULE_SET_ID = "air-quality-monitor"

class AirQualityMonitorRuleSetProvider : RuleSetProviderV3(RuleSetId(RULE_SET_ID)) {

    override fun getRuleProviders(): Set<RuleProvider> =
        setOf(
            RuleProvider { MultilineStatementSpacingRule() },
        )
}

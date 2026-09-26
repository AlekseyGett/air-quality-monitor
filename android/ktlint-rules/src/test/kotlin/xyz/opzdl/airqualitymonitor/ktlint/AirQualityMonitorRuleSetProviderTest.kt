package xyz.opzdl.airqualitymonitor.ktlint

import com.pinterest.ktlint.cli.ruleset.core.api.RuleSetProviderV3
import java.util.ServiceLoader
import org.junit.Assert.assertTrue
import org.junit.Test

class AirQualityMonitorRuleSetProviderTest {

    @Test
    fun `provider is discoverable through ServiceLoader`() {
        val providers = ServiceLoader.load(RuleSetProviderV3::class.java)

        assertTrue(providers.any { it is AirQualityMonitorRuleSetProvider })
    }
}

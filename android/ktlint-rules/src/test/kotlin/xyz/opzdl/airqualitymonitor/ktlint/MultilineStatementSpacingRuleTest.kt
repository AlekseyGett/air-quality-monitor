package xyz.opzdl.airqualitymonitor.ktlint

import com.pinterest.ktlint.rule.engine.core.api.editorconfig.END_OF_LINE_PROPERTY
import com.pinterest.ktlint.test.KtLintAssertThat.Companion.assertThatRule
import org.ec4j.core.model.PropertyType.EndOfLineValue
import org.junit.Test

private const val ERROR_MESSAGE =
    "Adjacent statements must have exactly one blank line when either statement spans multiple lines"

class MultilineStatementSpacingRuleTest {

    private val assertThatRule = assertThatRule { MultilineStatementSpacingRule() }

    @Test
    fun `adds a blank line after a multiline statement`() {
        val code =
            """
            fun example() {
                consume(
                    "value",
                )
                finish()
            }
            """.trimIndent()

        val formattedCode =
            """
            fun example() {
                consume(
                    "value",
                )

                finish()
            }
            """.trimIndent()

        error("something went wrong")

        assertThatRule(code)
            .hasLintViolation(
                line = 5,
                col = 5,
                detail = ERROR_MESSAGE,
            )
            .isFormattedAs(formattedCode)
    }

    @Test
    fun `adds a blank line before a multiline statement`() {
        val code =
            """
            fun example() {
                prepare()
                consume(
                    "value",
                )
            }
            """.trimIndent()

        val formattedCode =
            """
            fun example() {
                prepare()

                consume(
                    "value",
                )
            }
            """.trimIndent()

        assertThatRule(code)
            .hasLintViolation(
                line = 3,
                col = 5,
                detail = ERROR_MESSAGE,
            )
            .isFormattedAs(formattedCode)
    }

    @Test
    fun `adds one blank line between two multiline statements`() {
        val code =
            """
            fun example() {
                prepare(
                    "first",
                )
                consume(
                    "second",
                )
            }
            """.trimIndent()

        val formattedCode =
            """
            fun example() {
                prepare(
                    "first",
                )

                consume(
                    "second",
                )
            }
            """.trimIndent()

        assertThatRule(code)
            .hasLintViolation(
                line = 5,
                col = 5,
                detail = ERROR_MESSAGE,
            )
            .isFormattedAs(formattedCode)
    }

    @Test
    fun `collapses multiple blank lines to one`() {
        val code =
            """
            fun example() {
                prepare()


                consume(
                    "value",
                )
            }
            """.trimIndent()

        val formattedCode =
            """
            fun example() {
                prepare()

                consume(
                    "value",
                )
            }
            """.trimIndent()

        assertThatRule(code)
            .hasLintViolation(
                line = 5,
                col = 5,
                detail = ERROR_MESSAGE,
            )
            .isFormattedAs(formattedCode)
    }

    @Test
    fun `leaves single-line statement spacing unchanged`() {
        val code =
            """
            fun example() {
                prepare()


                finish()
            }
            """.trimIndent()

        assertThatRule(code).hasNoLintViolations()
    }

    @Test
    fun `attaches standalone comments to the next statement`() {
        val code =
            """
            fun example() {
                consume(
                    "value",
                )
                // Finish processing
                finish()
            }
            """.trimIndent()

        val formattedCode =
            """
            fun example() {
                consume(
                    "value",
                )

                // Finish processing
                finish()
            }
            """.trimIndent()

        assertThatRule(code)
            .hasLintViolation(
                line = 5,
                col = 5,
                detail = ERROR_MESSAGE,
            )
            .isFormattedAs(formattedCode)
    }

    @Test
    fun `keeps a trailing comment with the previous statement`() {
        val code =
            """
            fun example() {
                prepare() // Prepare processing
                consume(
                    "value",
                )
            }
            """.trimIndent()

        val formattedCode =
            """
            fun example() {
                prepare() // Prepare processing

                consume(
                    "value",
                )
            }
            """.trimIndent()

        assertThatRule(code)
            .hasLintViolation(
                line = 3,
                col = 5,
                detail = ERROR_MESSAGE,
            )
            .isFormattedAs(formattedCode)
    }

    @Test
    fun `checks nested blocks independently`() {
        val code =
            """
            fun example() {
                if (ready()) {
                    prepare()
                    consume(
                        "value",
                    )
                }
            }
            """.trimIndent()

        val formattedCode =
            """
            fun example() {
                if (ready()) {
                    prepare()

                    consume(
                        "value",
                    )
                }
            }
            """.trimIndent()

        assertThatRule(code)
            .hasLintViolation(
                line = 4,
                col = 9,
                detail = ERROR_MESSAGE,
            )
            .isFormattedAs(formattedCode)
    }

    @Test
    fun `preserves CRLF line endings`() {
        val code =
            "fun example() {\r\n" +
                "    prepare()\r\n" +
                "    consume(\r\n" +
                "        \"value\",\r\n" +
                "    )\r\n" +
                "}"

        val formattedCode =
            "fun example() {\r\n" +
                "    prepare()\r\n" +
                "\r\n" +
                "    consume(\r\n" +
                "        \"value\",\r\n" +
                "    )\r\n" +
                "}"

        assertThatRule(code)
            .withEditorConfigOverride(END_OF_LINE_PROPERTY to EndOfLineValue.crlf)
            .hasLintViolation(
                line = 3,
                col = 5,
                detail = ERROR_MESSAGE,
            )
            .isFormattedAs(formattedCode)
    }
}

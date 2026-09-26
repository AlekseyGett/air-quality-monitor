package xyz.opzdl.airqualitymonitor.ktlint

import com.pinterest.ktlint.rule.engine.core.api.AutocorrectDecision
import com.pinterest.ktlint.rule.engine.core.api.AutocorrectDecision.ALLOW_AUTOCORRECT
import com.pinterest.ktlint.rule.engine.core.api.Rule
import com.pinterest.ktlint.rule.engine.core.api.RuleAutocorrectApproveHandler
import com.pinterest.ktlint.rule.engine.core.api.RuleId
import com.pinterest.ktlint.rule.engine.core.api.indent
import com.pinterest.ktlint.rule.engine.core.api.isPartOfComment
import com.pinterest.ktlint.rule.engine.core.api.isWhiteSpace
import com.pinterest.ktlint.rule.engine.core.api.upsertWhitespaceBeforeMe
import org.jetbrains.kotlin.com.intellij.lang.ASTNode
import org.jetbrains.kotlin.psi.KtBlockExpression

private const val RULE_ID = "multiline-statement-spacing"
private const val REQUIRED_LINE_BREAKS = 2

private const val ERROR_MESSAGE =
    "Adjacent statements must have exactly one blank line when either statement spans multiple lines"

class MultilineStatementSpacingRule :
    Rule(ruleId = RuleId("$RULE_SET_ID:$RULE_ID"), about = About()),
    RuleAutocorrectApproveHandler {

    override fun beforeVisitChildNodes(
        node: ASTNode,
        emit: (offset: Int, errorMessage: String, canBeAutoCorrected: Boolean) -> AutocorrectDecision,
    ) {
        val statements = (node.psi as? KtBlockExpression)?.statements.orEmpty()

        statements.zipWithNext().forEach { (previousStatement, nextStatement) ->
            if (!previousStatement.text.isMultiline() && !nextStatement.text.isMultiline()) {
                return@forEach
            }

            val boundary = findNextStatementBoundary(previousStatement.node, nextStatement.node)
            val whitespace = boundary.treePrev.takeIf { it?.isWhiteSpace() == true }?.text.orEmpty()

            if (whitespace.lineBreakCount() == REQUIRED_LINE_BREAKS) {
                return@forEach
            }

            if (emit(boundary.startOffset, ERROR_MESSAGE, true) == ALLOW_AUTOCORRECT) {
                val indentation = whitespace.indentationAfterLastLineBreak() ?: previousStatement.node.indent()
                boundary.upsertWhitespaceBeforeMe("\n\n$indentation")
            }
        }
    }

    private fun findNextStatementBoundary(previousStatement: ASTNode, nextStatement: ASTNode): ASTNode {
        var sibling = previousStatement.treeNext

        while (sibling != null && sibling != nextStatement) {
            if (sibling.isPartOfComment() && hasLineBreakBefore(previousStatement, sibling)) {
                return sibling
            }

            sibling = sibling.treeNext
        }

        return nextStatement
    }

    private fun hasLineBreakBefore(previousStatement: ASTNode, comment: ASTNode): Boolean {
        var sibling = previousStatement.treeNext

        while (sibling != null && sibling != comment) {
            if (sibling.text.isMultiline()) {
                return true
            }

            sibling = sibling.treeNext
        }

        return false
    }

    private fun String.isMultiline(): Boolean = contains('\n') || contains('\r')

    private fun String.lineBreakCount(): Int = lineSequence().count().dec()

    private fun String.indentationAfterLastLineBreak(): String? {
        val lastLineBreak = maxOf(lastIndexOf('\n'), lastIndexOf('\r'))
        return lastLineBreak.takeIf { it >= 0 }?.let { substring(it + 1) }
    }
}

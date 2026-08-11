"""Unit tests for scripts/doxygen_doc_check.py."""
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))

import doxygen_doc_check  # noqa: E402


def violations(src, brief_only=False):
    """List of (kind, name) pairs reported for a source string."""
    return [(k, n) for (_, k, n, _)
            in doxygen_doc_check.check_source(src, brief_only=brief_only)[0]]


def kinds(src, brief_only=False):
    return [k for (k, _) in violations(src, brief_only=brief_only)]


FULL_DOC = """/**
 * @brief  Add two bytes.
 * @param  a  first operand
 * @param  b  second operand
 * @return the sum
 */
uint8_t add(uint8_t a, uint8_t b);
"""


class TestCompliance(unittest.TestCase):
    def test_fully_documented_passes(self):
        self.assertEqual(violations(FULL_DOC), [])

    def test_missing_doc_fails(self):
        src = "uint8_t add(uint8_t a, uint8_t b);\n"
        self.assertIn(("missing-doc", "add"), violations(src))

    def test_missing_brief_fails(self):
        src = """/**
 * Add two bytes.
 * @param  a  first operand
 * @param  b  second operand
 * @return the sum
 */
uint8_t add(uint8_t a, uint8_t b);
"""
        self.assertIn(("missing-brief", "add"), violations(src))

    def test_missing_one_param_fails(self):
        src = """/**
 * @brief  Add two bytes.
 * @param  a  first operand
 * @return the sum
 */
uint8_t add(uint8_t a, uint8_t b);
"""
        self.assertIn(("missing-param", "add"), violations(src))

    def test_extra_param_for_unnamed_param_fails(self):
        src = """/**
 * @brief  Reset the device.
 * @param  x  nothing to document
 */
void reset(void);
"""
        self.assertIn(("extra-param", "reset"), violations(src))

    def test_param_direction_bracket_fails(self):
        src = """/**
 * @brief  Write a byte.
 * @param[in]  data  byte to write
 */
void write(uint8_t data);
"""
        self.assertIn(("param-direction", "write"), violations(src))

    def test_nonvoid_without_return_fails(self):
        src = """/**
 * @brief  Add two bytes.
 * @param  a  first operand
 * @param  b  second operand
 */
uint8_t add(uint8_t a, uint8_t b);
"""
        self.assertIn(("missing-return", "add"), violations(src))

    def test_void_with_return_fails(self):
        src = """/**
 * @brief  Reset the device.
 * @return nothing
 */
void reset(void);
"""
        self.assertIn(("unexpected-return", "reset"), violations(src))

    def test_line_comment_doc_fails(self):
        src = """// Add two bytes.
uint8_t add(uint8_t a, uint8_t b);
"""
        self.assertIn(("wrong-doc-style", "add"), violations(src))

    def test_block_comment_doc_fails(self):
        src = """/* Add two bytes. */
uint8_t add(uint8_t a, uint8_t b);
"""
        self.assertIn(("wrong-doc-style", "add"), violations(src))

    def test_trailing_comment_is_not_a_doc(self):
        # A comment on the same line as code is a trailing note, not a doc
        # attempt: the next function has no doc at all.
        src = "uint8_t helper(void); /* trailing note */\nuint8_t add(uint8_t a, uint8_t b);\n"
        self.assertIn(("missing-doc", "add"), violations(src))

    def test_own_line_block_comment_is_a_doc_attempt(self):
        # A comment sitting alone in doc position counts as a (wrong-style)
        # doc attempt for the following function.
        src = "uint8_t helper(void);\n/* section note */\nuint8_t add(uint8_t a, uint8_t b);\n"
        self.assertIn(("wrong-doc-style", "add"), violations(src))


class TestSignatureForms(unittest.TestCase):
    def test_multiline_signature_passes(self):
        src = """/**
 * @brief  Compute a scaled value.
 * @param  a  input
 * @return scaled result
 */
static uint32_t
compute(uint16_t a);
"""
        self.assertEqual(violations(src), [])

    def test_multiline_param_list_passes(self):
        src = """/**
 * @brief  Init the peripheral.
 * @param  base  register base
 * @return status
 */
EPIC_StatusTypeDef epic_init(
    uint32_t base);
"""
        self.assertEqual(violations(src), [])

    def test_function_pointer_param_passes(self):
        src = """/**
 * @brief  Register a change callback.
 * @param  cb  callback invoked on change
 */
void register_cb(void (*cb)(uint8_t portb_value));
"""
        self.assertEqual(violations(src), [])

    def test_function_pointer_param_missing_doc_fails(self):
        src = """/**
 * @brief  Register a change callback.
 */
void register_cb(void (*cb)(uint8_t portb_value));
"""
        self.assertIn(("missing-param", "register_cb"), violations(src))

    def test_variadic_param_passes(self):
        src = """/**
 * @brief  Log a formatted message.
 * @param  fmt  format string
 */
void log_msg(const char *fmt, ...);
"""
        self.assertEqual(violations(src), [])

    def test_static_function_passes(self):
        src = """/**
 * @brief  Internal helper.
 * @param  a  operand
 * @return doubled operand
 */
static uint8_t helper(uint8_t a);
"""
        self.assertEqual(violations(src), [])

    def test_const_char_ptr_return_needs_return(self):
        src = """/**
 * @brief  Describe the part.
 * @return static description string
 */
const char *describe(void);
"""
        self.assertEqual(violations(src), [])

    def test_const_char_ptr_return_missing_return_fails(self):
        src = """/**
 * @brief  Describe the part.
 */
const char *describe(void);
"""
        self.assertIn(("missing-return", "describe"), violations(src))

    def test_void_ptr_return_is_nonvoid(self):
        src = """/**
 * @brief  Allocate scratch.
 * @param  n  byte count
 * @return pointer to scratch, or NULL
 */
void *alloc_scratch(uint8_t n);
"""
        self.assertEqual(violations(src), [])
        bad = """/**
 * @brief  Allocate scratch.
 * @param  n  byte count
 */
void *alloc_scratch(uint8_t n);
"""
        self.assertIn(("missing-return", "alloc_scratch"), violations(bad))

    def test_const_void_ptr_return_is_nonvoid(self):
        src = """/**
 * @brief  Peek a byte.
 * @param  addr  address
 * @return byte at addr
 */
const void *peek(uint16_t addr);
"""
        self.assertEqual(violations(src), [])

    def test_at_attribute_tolerated(self):
        src = """/**
 * @brief  Pinned multiply.
 * @param  a  operand
 * @return product
 */
uint16_t pinned_mul(uint8_t a) __at(0x100);
"""
        self.assertEqual(violations(src), [])

    def test_epic_place_macro_tolerated(self):
        src = """/**
 * @brief  Pinned multiply.
 * @param  a  operand
 * @return product
 */
uint16_t pinned_mul(uint8_t a) EPIC_PLACE(0x100);
"""
        self.assertEqual(violations(src), [])

    def test_weak_attribute_tolerated(self):
        src = """/**
 * @brief  Weak IRQ handler.
 */
void TIMER1_IRQHandler(void) EPIC_WEAK;
"""
        self.assertEqual(violations(src), [])
        src2 = """/**
 * @brief  Weak IRQ handler.
 */
void TIMER2_IRQHandler(void) __attribute__((weak));
"""
        self.assertEqual(violations(src2), [])

    def test_interrupt_isr_tolerated(self):
        src = """/**
 * @brief  ISR for the peripheral.
 */
void __interrupt() PIC16_IRQ_Handler(void);
"""
        self.assertEqual(violations(src), [])

    def test_interrupt_with_priority_arg_tolerated(self):
        src = """/**
 * @brief  High-priority ISR.
 */
void __interrupt(high_priority) PIC18_IRQ_HandlerHigh(void);
"""
        self.assertEqual(violations(src), [])

    def test_void_param_needs_no_param(self):
        src = """/**
 * @brief  Reset the device.
 */
void reset(void);
"""
        self.assertEqual(violations(src), [])

    def test_empty_parens_needs_no_param(self):
        src = """/**
 * @brief  Legacy entry point.
 */
void legacy();
"""
        self.assertEqual(violations(src), [])

    def test_struct_pointer_param(self):
        src = """/**
 * @brief  Open a stream.
 * @param  s  stream to open
 * @return status
 */
int open_stream(struct stream *s);
"""
        self.assertEqual(violations(src), [])


class TestBriefOnly(unittest.TestCase):
    def test_brief_only_passes_brief_only_block(self):
        src = """/**
 * @brief  Add two bytes.
 */
uint8_t add(uint8_t a, uint8_t b);
"""
        self.assertEqual(violations(src, brief_only=True), [])
        self.assertIn(("missing-return", "add"), violations(src))

    def test_brief_only_still_requires_brief(self):
        src = """/**
 * Add two bytes.
 */
uint8_t add(uint8_t a, uint8_t b);
"""
        self.assertIn(("missing-brief", "add"), violations(src, brief_only=True))

    def test_brief_only_still_requires_doc(self):
        src = "uint8_t add(uint8_t a, uint8_t b);\n"
        self.assertIn(("missing-doc", "add"), violations(src, brief_only=True))


class TestFailClosed(unittest.TestCase):
    def test_unbalanced_parens_unparseable(self):
        src = "uint8_t bad(uint8_t a;\n"
        self.assertIn(("unparseable", "bad"), violations(src))

    def test_garbage_tail_unparseable(self):
        src = "uint8_t bad(uint8_t a) ??? ;\n"
        self.assertIn(("unparseable", "bad"), violations(src))

    def test_nested_function_typed_param_not_a_function(self):
        # A function-typed parameter (no `*`) is part of the outer signature,
        # never reported on its own.
        src = """/**
 * @brief  Apply a mapper.
 * @param  mapper  mapping function
 * @return status
 */
int apply(int mapper(int x));
"""
        self.assertEqual(violations(src), [])

    def test_file_scope_macro_call_not_a_function(self):
        src = "STATIC_SIZE_CHECK_EQUAL(USB_ARRAYLEN(A), B);\n"
        self.assertEqual(violations(src), [])

    def test_doc_separated_by_code_is_missing(self):
        src = """/**
 * @brief  Doc for helper.
 */
uint8_t helper(void);
/**
 * @brief  Doc for add.
 */
uint8_t other(void);
uint8_t add(uint8_t a, uint8_t b);
"""
        self.assertIn(("missing-doc", "add"), violations(src))
        self.assertNotIn(("missing-doc", "helper"), violations(src))
        self.assertNotIn(("missing-doc", "other"), violations(src))

    def test_words_inside_strings_are_not_functions(self):
        src = 'void log_all(void) { logf("encoder A (RB4/RB5): ok\\n"); }\n'
        self.assertEqual(violations(src), [("missing-doc", "log_all")])

    def test_static_function_pointer_variable_is_not_a_function(self):
        src = "static void (*g_cb)(void) = NULL;\n"
        self.assertEqual(violations(src), [])

    def test_preprocessor_conditional_signature_variants(self):
        # The same function appears in both arms of a #if/#else with a
        # different attribute; both are real functions and neither may be
        # reported as unparseable.
        src = """#if defined(__XC8)
void dispatch(void) __at(0x900)
#else
void dispatch(void)
#endif
{
    (void)0;
}
"""
        kinds_seen = [k for (k, _) in violations(src)]
        self.assertNotIn("unparseable", kinds_seen)
        self.assertEqual(kinds_seen.count("missing-doc"), 2)


if __name__ == "__main__":
    unittest.main()

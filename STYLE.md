
# C coding style

Follow the
[OpenBSD style(9)](http://man.openbsd.org/OpenBSD-current/man9/style.9),
except for the differences noted below.

## Differences to style(9)

When testing a pointer for NULL, rely on conversion to boolean.
That is, use `if (p)` or `if (!p)`.

`sizeof` is a keyword and should be followed by a space.
Do not parenthesize its argument unless it is a type,
or a complicated expression.

## Clarifications to style(9)

Use a single eight-column TABs to indent.
Statements that continue over a line are indented once by four spaces
and no further on subsequent lines.
Avoid using TABs within comments, except for the initial indent.

Braces around a single-line statement should be used if the line
is near comments or complicated logic, or the statement is not trivial,
or is unlikely to remain trivial.

Use C99, and the following GCC extensions:
 * anonymous structures and unions (from C11)

Use GCC attribute extensions only if #defining them away would still
allow the code to function.

Avoid `alloca()`. Avoid variable-length arrays (VLAs). If you need these,
prefer your caller to provide a buffer.

# Unit tests

Unit tests should be placed in simple programs whose name begins with `t-`.
These programs are self-contained and may be run in parallel by
`make check`.

Each test program should be a simple `main()` function that sets up
the functions under test, and uses `assert()` for every expected outcome.

Keep test programs simple.
They should not need arguments.
They should not over-test.

Avoid too many levels of preprocessor macros.
Macros are handy for exercising multiple aspects of a function, but
because test programs are used for debugging ("Why did my change over *here*
cause that test to fail over *there*?") it should be clear how to
run the test in a debugger and step into the failure.

# Comment style

Comments should use C syntax `/* ... */`

Documentation comments for public symbols (functions, types, global variables)
are kept immediately prior to their declarations in a header file.

Private symbols are documented in comments immediately prior
to their implementation in code.

Function documentation must contain sufficient information for someone to
write tests without looking at the implementation code.

## Comment documentation style

Comments for public symbols use
[doxygen markup](https://www.stack.nl/~dimitri/doxygen/manual/commands.html).

All doxygen comments are introduced with a plain `/**` line, subsequent lines
indented with a single `*`, and closed on a separate line with `*/`.
Use `@` to introduce doxygen commands.

```c
/**
 * Adds two integers.
 *
 * @param a  first addend
 * @param b  second addend
 *
 * @returns sum of the addends
 */
```

This style for comments is based on
[Java's conventions](http://www.oracle.com/technetwork/java/javase/documentation/index-137868.html).

The first words of a documentation comment are a brief summary
in the form of a verb clause.
It starts with an active, present-tense verb and ends with a period `.`.
The summary should not exceed one line, and must never exceed two.

After the summary comes the description proper.
It is always preceded by a blank line.

Next come the parameters, in order of declaration.
These are separated from the summary by a blank line.
Individually they should not be separated by blank lines.

Next come the return values.
The return values should be grouped as a block preceded by
a blank line but without interior blank lines.
The `@returns` should come first, followed by any `@retval` lines.
Use `@returns` instead of `@return`.

Finally come the warnings and `@since` declarations.

## Sentence style

Use sentences beginning with a capital letter and terminating with
a period (`.`), except for `@param` and `@returns`.

The `@param` and `@returns` lines should be followed by a noun clause.
If they can fit on the same line, omit the leading article.
The terminating period must be omitted unless there are following
sentences.

The `@returns` and `@retval` lines should refer to
parameters and system state in the present tense,
but should refer to the operation of the function in the past tense.

## Describe beyond the name

The text should provide a description that is "beyond the name".
This is done by avoiding using the words that are already in the
parameter name or API name.
A parameter description should make reference to *how* the
parameter is used.
A return description should refer to what the return value *means*
to the caller.

## Error values

If a function returns an error value and sets `errno`, indicate the
different errors in `@retval` lines that immediately includes the error
value in square brackets. Example:

```c
/**
 * Promises to send data.
 *
 * @param      src  pointer to the data to send
 * @param[in]  n    size of the data to send, in bytes
 * @param[out] n    number of bytes actually sent. This will
 *                  never be zero.
 *
 * @returns a #promise
 * @retval NULL [EINVAL]
 *     The source pointer was not aligned to a word boundary.
 */
```

## Economic linking

Use links economically. Only the first occurrence of a linkable identifier
should be preceded with `#`.

Do not link identifiers from the standard library. Use `@c` instead.

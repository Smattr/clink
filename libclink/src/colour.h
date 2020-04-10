#pragma once

/** convert a 6-character HTML hex colour into its ANSI code equivalent
 *
 * \param html String of at least 6 hex characters representing an HTML colour
 * \returns the closest ANSI colour code
 */
__attribute__((visibility("internal")))
unsigned html_colour_to_ansi(const char *html);

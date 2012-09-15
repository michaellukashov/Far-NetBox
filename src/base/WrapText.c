#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/*----------------------------------------------------------------------------
 * return the next available word, ignoring whitespace
 */
static const wchar_t *
NextWord(const wchar_t * input)
{
  static wchar_t buffer[1024];
  static const wchar_t * text = 0;

  wchar_t * endOfBuffer = buffer + sizeof(buffer) - 1;
  wchar_t * pBuffer = buffer;

  if (input)
  {
    text = input;
  }

  if (text)
  {
    /* skip leading spaces */
    while (iswspace(*text))
    {
      ++text;
    }

    /* copy the word to our static buffer */
    while (*text && !iswspace(*text) && pBuffer < endOfBuffer)
    {
      *(pBuffer++) = *(text++);
    }
  }

  *pBuffer = 0;

  return buffer;
}

/*----------------------------------------------------------------------------
 */
const wchar_t *
WrapText(const wchar_t * text,
  int maxWidth,
  const wchar_t * prefixFirst,
  const wchar_t * prefixRest)
{
  const wchar_t * prefix = 0;
  const wchar_t * s = 0;
  wchar_t * wrap = 0;
  wchar_t * w = 0;

  int lineCount = 0;
  int lenBuffer = 0;
  int lenPrefixFirst = wcslen(prefixFirst ? prefixFirst : L"");
  int lenPrefixRest = wcslen(prefixRest ? prefixRest : L"");
  int spaceLeft = maxWidth;
  int wordsThisLine = 0;

  if (maxWidth == 0)
  {
    maxWidth = 78;
  }
  if (lenPrefixFirst + 5 > maxWidth)
  {
    maxWidth = lenPrefixFirst + 5;
  }
  if (lenPrefixRest + 5 > maxWidth)
  {
    maxWidth = lenPrefixRest + 5;
  }

  /* two passes through the input. the first pass updates the buffer length.
   * the second pass creates and populates the buffer
   */
  while (wrap == 0)
  {
    lineCount = 0;

    if (lenBuffer)
    {
      /* second pass, so create the wrapped buffer */
      wrap = (wchar_t *)malloc(sizeof(wchar_t) * (lenBuffer + 1));
      if (wrap == 0)
      {
        break;
      }
    }
    w = wrap;

    /* for each Word in Text
     *   if Width(Word) > SpaceLeft
     *     insert line break before Word in Text
     *     SpaceLeft := LineWidth - Width(Word)
     *   else
     *     SpaceLeft := SpaceLeft - Width(Word) + SpaceWidth
     */
    s = NextWord(text);
    while (*s)
    {
      spaceLeft = maxWidth;
      wordsThisLine = 0;

      /* copy the prefix */
      prefix = lineCount ? prefixRest : prefixFirst;
      prefix = prefix ? prefix : L"";
      while (*prefix)
      {
        if (w == 0)
        {
          ++lenBuffer;
        }
        else
        {
          *(w++) = *prefix == '\n' ? ' ' : *prefix;
        }
        --spaceLeft;
        ++prefix;
      }

      /* force the first word to always be completely copied */
      while (*s)
      {
        if (w == 0)
        {
          ++lenBuffer;
        }
        else
        {
          *(w++) = *s;
        }
        --spaceLeft;
        ++s;
      }
      if (!*s)
      {
        s = NextWord(0);
      }

      /* copy as many words as will fit onto the current line */
      while (*s && wcslen(s) + 1 <= spaceLeft)
      {
        /* will fit so add a space between the words */
        if (w == 0)
        {
          ++lenBuffer;
        }
        else
        {
          *(w++) = ' ';
        }
        --spaceLeft;

        /* then copy the word */
        while (*s)
        {
          if (w == 0)
          {
            ++lenBuffer;
          }
          else
          {
            *(w++) = *s;
          }
          --spaceLeft;
          ++s;
        }
        if (!*s)
        {
          s = NextWord(0);
        }
      }
      if (!*s)
      {
        s = NextWord(0);
      }

      if (*s)
      {
        /* add a new line here */
        if (w == 0)
        {
          ++lenBuffer;
        }
        else
        {
          *(w++) = '\n';
        }
      }

      ++lineCount;
    }

    lenBuffer += 2;

    if (w)
    {
      *w = 0;
    }
  }

  return wrap;
}

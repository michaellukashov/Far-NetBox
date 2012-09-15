#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/*----------------------------------------------------------------------------
 * return the next available word, ignoring whitespace
 */
static const char *
NextWord(const char * input)
{
  static char buffer[1024];
  static const char * text = 0;

  char * endOfBuffer = buffer + sizeof(buffer) - 1;
  char * pBuffer = buffer;

  if(input)
  {
    text = input;
  }

  if(text)
  {
    /* skip leading spaces */
    while(isspace(*text))
    {
      ++text;
    }

    /* copy the word to our static buffer */
    while(*text && !isspace(*text) && pBuffer < endOfBuffer)
    {
      *(pBuffer++) = *(text++);
    }
  }

  *pBuffer = 0;

  return buffer;
}

/*----------------------------------------------------------------------------
 */
const char *
WrapText(const char * text,
  int maxWidth,
  const char * prefixFirst,
  const char * prefixRest)
{
  const char * prefix = 0;
  const char * s = 0;
  char * wrap = 0;
  char * w = 0;

  int lineCount = 0;
  int lenBuffer = 0;
  int lenPrefixFirst = strlen(prefixFirst? prefixFirst: "");
  int lenPrefixRest = strlen(prefixRest ? prefixRest : "");
  int spaceLeft = maxWidth;
  int wordsThisLine = 0;

  if(maxWidth == 0)
  {
    maxWidth = 78;
  }
  if(lenPrefixFirst + 5 > maxWidth)
  {
    maxWidth = lenPrefixFirst + 5;
  }
  if(lenPrefixRest + 5 > maxWidth)
  {
    maxWidth = lenPrefixRest + 5;
  }

  /* two passes through the input. the first pass updates the buffer length.
   * the second pass creates and populates the buffer
   */
  while(wrap == 0)
  {
    lineCount = 0;

    if(lenBuffer)
    {
      /* second pass, so create the wrapped buffer */
      wrap = (char *)malloc(sizeof(char) * (lenBuffer + 1));
      if(wrap == 0)
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
    while(*s)
    {
      spaceLeft = maxWidth;
      wordsThisLine = 0;

      /* copy the prefix */
      prefix = lineCount ? prefixRest : prefixFirst;
      prefix = prefix ? prefix : "";
      while(*prefix)
      {
        if(w == 0)
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
      while(*s)
      {
        if(w == 0)
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
      if(!*s)
      {
        s = NextWord(0);
      }

      /* copy as many words as will fit onto the current line */
      while(*s && strlen(s) + 1 <= spaceLeft)
      {
        /* will fit so add a space between the words */
        if(w == 0)
        {
          ++lenBuffer;
        }
        else
        {
          *(w++) = ' ';
        }
        --spaceLeft;

        /* then copy the word */
        while(*s)
        {
          if(w == 0)
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
        if(!*s)
        {
          s = NextWord(0);
        }
      }
      if(!*s)
      {
        s = NextWord(0);
      }

      if(*s)
      {
        /* add a new line here */
        if(w == 0)
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

    if(w)
    {
      *w = 0;
    }
  }

  return wrap;
}

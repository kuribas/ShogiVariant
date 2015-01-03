#include <pnm.h>
#include <pbm.h>
#include <stdio.h>
#include <getopt.h>

bit outsideColor, insideColor, borderColor, textColor;
const char* helpmessage = "Gebruik: %s [OPTIE] ... [BESTAND]\n"
"  -t  verkrijg text (standaard)\n"
"  -b  verkrijg rand\n"
"  -p  verkrijg stuk\n"
"  -i  verkrijg binnenkand van stuk (min rand)\n"
"  -c  beschrijving: aangepast in de vorm XXXX met X = w of b\n"
"      ( w is wit, b is zwart) en in volgorde:\n"
"        - buitenkleur, binnenkleur, randkleur, tekstkleur\n";
         

int setCustom(const char *str)
{
  if(*str == 'w') outsideColor = PBM_WHITE;
  else if(*str == 'b') outsideColor = PBM_BLACK;
  else return 0;

  str++;
  if(*str == 'w') insideColor = PBM_WHITE;
  else if(*str == 'b') insideColor = PBM_BLACK;
  else return 0;
    
  str++;
  if(*str == 'w') borderColor = PBM_WHITE;
  else if(*str == 'b') borderColor = PBM_BLACK;
  else return 0;

  str++;
  if(*str == 'w') textColor = PBM_WHITE;
  else if(*str == 'b') textColor = PBM_BLACK;
  else return 0;

  if(*++str == 0)return 1;
  else return 0;
}

int main(int argc, char **argv)
{
  int cols, rows, format, c;
  int minInsideCol, maxInsideCol;
  int iRow, iCol;
  int nColor = 0;
  int optionset = 0;

  xelval maxval;
  xel back1, back2, border;
  bit *outrow;
  xel *inrow;

  pnm_init(&argc, argv);
  pbm_init(&argc, argv);
  
  outsideColor = insideColor = borderColor = PBM_BLACK;
  textColor = PBM_WHITE;

  while(1)
    {
      c = getopt(argc, argv, "tibphc:");
      if(c == -1)break;
      
      switch(c)
	{
	case 'h':
	  fprintf(stdout, helpmessage, argv[0]);
	  return 0;
	case 't':
	  /* text: doe niets, is default */
	  break;
	case 'i':
	  /* inside */
	  outsideColor = borderColor = PBM_WHITE;
	  textColor = insideColor = PBM_BLACK;
	  break;
	case 'b':
	  /* border */
	  outsideColor = textColor = insideColor = PBM_WHITE;
	  borderColor = PBM_BLACK;
	  break;
	case 'p':
	  /* piece */
	  outsideColor = PBM_WHITE;
	  borderColor = textColor = insideColor = PBM_BLACK;
	  break;
	case 'c':
	  if( !setCustom(optarg) )
	    {
	      fprintf(stderr, "Fout: onjuist argument voor custom (-c): %s\n", optarg);
	      return 1;
	    }
	  break;
	case ':':
	  fprintf(stderr, "Fout: custom(-c) verwacht een argument.\n");
	  return 1;
	default:
	  fprintf(stderr, helpmessage, argv[0]);
	  return 1;
	}

      if(optionset)
	{
	  fprintf(stderr, "Fout: slechts een optie van -tibpc "
		"kan gebruikt worden.\n");
	  return 1;
	}
      else
	optionset = 1;
    }

  pnm_readpnminit(stdin, &cols, &rows, &maxval, &format);
  pbm_writepbminit(stdout, cols, rows, 0);
  
  outrow = pbm_allocrow( cols);
  inrow = pnm_allocrow( cols );

  minInsideCol = cols;
  maxInsideCol = -1;
  for(iRow = 0; iRow < rows; iRow++)
    {
      pnm_readpnmrow(stdin, inrow, cols, maxval, format);
      if(nColor == 0)
	{ back1 = inrow[0]; nColor++; }

      /* scan achterwaarts tot maxInsideCol */
      for(iCol = cols - 1; iCol >= 0; iCol--)
	{
	  if(PNM_EQUAL(inrow[iCol], back1))
	    outrow[iCol] = outsideColor;
	  else break;
	}
      /* voorbij de eerste kolom */
      if(iCol == -1)goto nextrow;
      
      /* nieuwe kleur */
      if(nColor == 1)
	{ border = inrow[iCol]; nColor++; }

      for(; iCol > maxInsideCol; iCol--)
	{
	  if(PNM_EQUAL(inrow[iCol], border))
	    outrow[iCol] = borderColor;	  
	  else break;
	}
      maxInsideCol = iCol;

      /* scan voorwaarts to minInsideCol */
      for(iCol = 0; iCol < cols; iCol++)
	{
	  if(PNM_EQUAL(inrow[iCol], back1))
	    outrow[iCol] = outsideColor;
	  else break;
	}

      /* voorbij de laatste kolom */
      if(iCol == cols)goto nextrow;

      for(; iCol < minInsideCol; iCol++)
	{
	  if(PNM_EQUAL(inrow[iCol], border))
	    outrow[iCol] = borderColor;	  
	  else break;
	}
      minInsideCol = iCol;
      if(maxInsideCol < minInsideCol)goto nextrow;

      if(nColor == 2);
      { back2 = inrow[iCol]; nColor++; }
	 
      /* scan binnenkant */
      /* iCol = minInsideCol */
      for(; iCol <= maxInsideCol; iCol++)
	{
	  if(PNM_EQUAL(inrow[iCol], back2))
	    outrow[iCol] = insideColor;
	  else
	    outrow[iCol] = textColor;
	}

    nextrow:
      pbm_writepbmrow(stdout, outrow, cols, 0);
    }

  pbm_freerow(outrow);
  pnm_freerow( inrow );

  return 0;
}
  

  

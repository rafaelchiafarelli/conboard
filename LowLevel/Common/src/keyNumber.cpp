#include <keyNumbers.hpp>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
unsigned int lhash ( const char *str,  size_t len)
{
  static unsigned short asso_values[] =
    {
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 103, 173,
      206,  63,  52, 131,  81, 226,  29,   9, 156, 993,
      993, 993, 993, 993, 993,  68, 129, 132, 196, 128,
       14, 215, 152, 239, 993,  44, 993, 137, 192, 179,
       13,   6, 164,  79, 198, 152,  30, 993, 993, 156,
      993, 200, 993, 993, 993, 993, 252, 170, 121, 193,
      199,  83, 150, 182, 194, 160,  93, 115, 100,   7,
      240,  51, 152, 192, 155, 138, 253, 147, 198,  15,
      125,  77, 249,  77, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993, 993, 993, 993,
      993, 993, 993, 993, 993, 993, 993
    };
   unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[2]+1];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]+1];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

namedKeyCodes in_word_set ( const char *str,  size_t len)
{
      namedKeyCodes ret = {.name = NULL, .number = 0};
  enum
    {
      TOTAL_KEYWORDS = 232,
      MIN_WORD_LENGTH = 2,
      MAX_WORD_LENGTH = 16,
      MIN_HASH_VALUE = 190,
      MAX_HASH_VALUE = 992
    };

  static namedKeyCodes wordlist[] =
    {
//#line 78 "keyNumbers.hash"
      {"f8",65},
//#line 102 "keyNumbers.hash"
      {"kOne",89},
//#line 73 "keyNumbers.hash"
      {"f3",60},
//#line 100 "keyNumbers.hash"
      {"kPlus",87},
//#line 165 "keyNumbers.hash"
      {"lang9",152},
//#line 58 "keyNumbers.hash"
      {"minus",45},
//#line 214 "keyNumbers.hash"
      {"kPipe",201},
//#line 164 "keyNumbers.hash"
      {"lang8",151},
//#line 79 "keyNumbers.hash"
      {"f9",66},
//#line 208 "keyNumbers.hash"
      {"kExponential",195},
//#line 160 "keyNumbers.hash"
      {"lang4",147},
//#line 127 "keyNumbers.hash"
      {"f23",114},
//#line 74 "keyNumbers.hash"
      {"f4",61},
//#line 135 "keyNumbers.hash"
      {"undo",122},
//#line 159 "keyNumbers.hash"
      {"lang3",146},
//#line 101 "keyNumbers.hash"
      {"kEnter",88},
//#line 162 "keyNumbers.hash"
      {"lang6",149},
//#line 131 "keyNumbers.hash"
      {"menu",118},
//#line 75 "keyNumbers.hash"
      {"f5",62},
//#line 209 "keyNumbers.hash"
      {"kPercentage",196},
//#line 156 "keyNumbers.hash"
      {"inter9",143},
//#line 189 "keyNumbers.hash"
      {"kDzero",176},
//#line 63 "keyNumbers.hash"
      {"Tilde",50},
//#line 93 "keyNumbers.hash"
      {"lArrow",80},
//#line 155 "keyNumbers.hash"
      {"inter8",142},
//#line 116 "keyNumbers.hash"
      {"kEqual0",103},
//#line 122 "keyNumbers.hash"
      {"f18",109},
//#line 169 "keyNumbers.hash"
      {"Clear0",156},
//#line 128 "keyNumbers.hash"
      {"f24",115},
//#line 129 "keyNumbers.hash"
      {"execute",116},
//#line 190 "keyNumbers.hash"
      {"kTzero",177},
//#line 170 "keyNumbers.hash"
      {"Prior",157},
//#line 230 "keyNumbers.hash"
      {"kClearEntry",217},
//#line 77 "keyNumbers.hash"
      {"f7",64},
//#line 161 "keyNumbers.hash"
      {"lang5",148},
//#line 168 "keyNumbers.hash"
      {"Cancel",155},
//#line 29 "keyNumbers.hash"
      {"letter_m",16},
//#line 213 "keyNumbers.hash"
      {"kDoubleECom",200},
//#line 166 "keyNumbers.hash"
      {"altErase",153},
//#line 151 "keyNumbers.hash"
      {"inter4",138},
//#line 90 "keyNumbers.hash"
      {"end",77},
//#line 39 "keyNumbers.hash"
      {"letter_w",26},
//#line 72 "keyNumbers.hash"
      {"f2",59},
//#line 231 "keyNumbers.hash"
      {"kBinary",218},
//#line 150 "keyNumbers.hash"
      {"inter3",137},
//#line 65 "keyNumbers.hash"
      {"singleCuotes",52},
//#line 232 "keyNumbers.hash"
      {"kOctal",219},
//#line 95 "keyNumbers.hash"
      {"uArrow",82},
//#line 212 "keyNumbers.hash"
      {"kECommertial",199},
//#line 140 "keyNumbers.hash"
      {"mute",127},
//#line 53 "keyNumbers.hash"
      {"enter",40},
//#line 55 "keyNumbers.hash"
      {"deleteKey",42},
//#line 92 "keyNumbers.hash"
      {"rArrow",79},
//#line 153 "keyNumbers.hash"
      {"inter6",140},
//#line 103 "keyNumbers.hash"
      {"kTwo",90},
//#line 157 "keyNumbers.hash"
      {"lang1",144},
//#line 207 "keyNumbers.hash"
      {"kXOR",194},
//#line 31 "keyNumbers.hash"
      {"letter_o",18},
//#line 220 "keyNumbers.hash"
      {"kExclamation",207},
//#line 76 "keyNumbers.hash"
      {"f6",63},
//#line 200 "keyNumbers.hash"
      {"kBackspace",187},
//#line 147 "keyNumbers.hash"
      {"kEqual1",134},
//#line 175 "keyNumbers.hash"
      {"Clear1",162},
//#line 198 "keyNumbers.hash"
      {"kCloseBrakets",185},
//#line 197 "keyNumbers.hash"
      {"kOpenBrakets",184},
//#line 196 "keyNumbers.hash"
      {"kCloseParentesis",183},
//#line 195 "keyNumbers.hash"
      {"kOpenParentesis",182},
//#line 117 "keyNumbers.hash"
      {"f13",104},
//#line 110 "keyNumbers.hash"
      {"kNine",97},
//#line 229 "keyNumbers.hash"
      {"kClear",216},
//#line 89 "keyNumbers.hash"
      {"kDeleteKey",76},
//#line 109 "keyNumbers.hash"
      {"kEight",96},
//#line 41 "keyNumbers.hash"
      {"letter_y",28},
//#line 158 "keyNumbers.hash"
      {"lang2",145},
//#line 126 "keyNumbers.hash"
      {"f22",113},
//#line 94 "keyNumbers.hash"
      {"dArrow",81},
//#line 21 "keyNumbers.hash"
      {"letter_e",8},
//#line 215 "keyNumbers.hash"
      {"kDoublePipe",202},
//#line 124 "keyNumbers.hash"
      {"f20",111},
//#line 239 "keyNumbers.hash"
      {"lAlt",226},
//#line 152 "keyNumbers.hash"
      {"inter5",139},
//#line 139 "keyNumbers.hash"
      {"find",126},
//#line 26 "keyNumbers.hash"
      {"letter_j",13},
//#line 233 "keyNumbers.hash"
      {"kDecimal",220},
//#line 163 "keyNumbers.hash"
      {"lang7",150},
//#line 28 "keyNumbers.hash"
      {"letter_l",15},
//#line 130 "keyNumbers.hash"
      {"help",117},
//#line 177 "keyNumbers.hash"
      {"ExSel",164},
//#line 106 "keyNumbers.hash"
      {"kFive",93},
//#line 199 "keyNumbers.hash"
      {"kTab",186},
//#line 141 "keyNumbers.hash"
      {"vol_up",128},
//#line 104 "keyNumbers.hash"
      {"kThree",91},
//#line 27 "keyNumbers.hash"
      {"letter_k",14},
//#line 111 "keyNumbers.hash"
      {"kZero",98},
//#line 206 "keyNumbers.hash"
      {"kHexF",193},
//#line 123 "keyNumbers.hash"
      {"f19",110},
//#line 240 "keyNumbers.hash"
      {"lGUI",227},
//#line 18 "keyNumbers.hash"
      {"letter_b",5},
//#line 40 "keyNumbers.hash"
      {"letter_x",27},
//#line 71 "keyNumbers.hash"
      {"f1",58},
//#line 107 "keyNumbers.hash"
      {"kSix",94},
//#line 148 "keyNumbers.hash"
      {"inter1",135},
//#line 216 "keyNumbers.hash"
      {"kTwoDots",203},
//#line 57 "keyNumbers.hash"
      {"space",44},
//#line 59 "keyNumbers.hash"
      {"equal",46},
//#line 118 "keyNumbers.hash"
      {"f14",105},
//#line 35 "keyNumbers.hash"
      {"letter_s",22},
//#line 99 "keyNumbers.hash"
      {"kMinus",86},
//#line 210 "keyNumbers.hash"
      {"kLess",197},
//#line 226 "keyNumbers.hash"
      {"kMemMultiply",213},
//#line 211 "keyNumbers.hash"
      {"kMode",198},
//#line 243 "keyNumbers.hash"
      {"rAlt",230},
//#line 221 "keyNumbers.hash"
      {"kMemStore",208},
//#line 227 "keyNumbers.hash"
      {"kMemDivide",214},
//#line 37 "keyNumbers.hash"
      {"letter_u",24},
//#line 193 "keyNumbers.hash"
      {"Currency",180},
//#line 132 "keyNumbers.hash"
      {"select",119},
//#line 22 "keyNumbers.hash"
      {"letter_f",9},
//#line 237 "keyNumbers.hash"
      {"lControl",224},
//#line 32 "keyNumbers.hash"
      {"letter_p",19},
//#line 85 "keyNumbers.hash"
      {"pause",72},
//#line 34 "keyNumbers.hash"
      {"letter_r",21},
//#line 144 "keyNumbers.hash"
      {"lock_num_l",131},
//#line 143 "keyNumbers.hash"
      {"lock_caps_l",130},
//#line 145 "keyNumbers.hash"
      {"lock_scroll_l",132},
//#line 25 "keyNumbers.hash"
      {"letter_i",12},
//#line 222 "keyNumbers.hash"
      {"kMemRecall",209},
//#line 219 "keyNumbers.hash"
      {"Kat",206},
//#line 149 "keyNumbers.hash"
      {"inter2",136},
//#line 119 "keyNumbers.hash"
      {"f15",106},
//#line 113 "keyNumbers.hash"
      {"kBackSlash",100},
//#line 176 "keyNumbers.hash"
      {"CrSel",163},
//#line 17 "keyNumbers.hash"
      {"letter_a",4},
//#line 183 "keyNumbers.hash"
      {"reserved9",170},
//#line 201 "keyNumbers.hash"
      {"kHexA",188},
//#line 244 "keyNumbers.hash"
      {"rGUI",231},
//#line 60 "keyNumbers.hash"
      {"oBracket",47},
//#line 228 "keyNumbers.hash"
      {"kChangeSignal",215},
//#line 172 "keyNumbers.hash"
      {"Separator",159},
//#line 23 "keyNumbers.hash"
      {"letter_g",10},
//#line 115 "keyNumbers.hash"
      {"power",102},
//#line 154 "keyNumbers.hash"
      {"inter7",141},
//#line 98 "keyNumbers.hash"
      {"kMultiply",85},
//#line 133 "keyNumbers.hash"
      {"stop",120},
//#line 218 "keyNumbers.hash"
      {"kSpace",205},
//#line 125 "keyNumbers.hash"
      {"f21",112},
//#line 182 "keyNumbers.hash"
      {"reserved8",169},
//#line 33 "keyNumbers.hash"
      {"letter_q",20},
//#line 19 "keyNumbers.hash"
      {"letter_c",6},
//#line 24 "keyNumbers.hash"
      {"letter_h",11},
//#line 84 "keyNumbers.hash"
      {"scrollLock",71},
//#line 38 "keyNumbers.hash"
      {"letter_v",25},
//#line 20 "keyNumbers.hash"
      {"letter_d",7},
//#line 142 "keyNumbers.hash"
      {"vol_down",129},
//#line 241 "keyNumbers.hash"
      {"rControl",228},
//#line 121 "keyNumbers.hash"
      {"f17",108},
//#line 137 "keyNumbers.hash"
      {"copy",124},
//#line 178 "keyNumbers.hash"
      {"reserved4",165},
//#line 188 "keyNumbers.hash"
      {"reserved14",175},
//#line 223 "keyNumbers.hash"
      {"kMemClear",210},
//#line 54 "keyNumbers.hash"
      {"escape",41},
//#line 88 "keyNumbers.hash"
      {"pageUp",75},
//#line 52 "keyNumbers.hash"
      {"n_zero",39},
//#line 82 "keyNumbers.hash"
      {"f12",69},
//#line 70 "keyNumbers.hash"
      {"capslock",57},
//#line 16 "keyNumbers.hash"
      {"reserved3",3},
//#line 187 "keyNumbers.hash"
      {"reserved13",174},
//#line 51 "keyNumbers.hash"
      {"n_nine",38},
//#line 83 "keyNumbers.hash"
      {"printScreen",70},
//#line 80 "keyNumbers.hash"
      {"f10",67},
//#line 205 "keyNumbers.hash"
      {"kHexE",192},
//#line 202 "keyNumbers.hash"
      {"kHexB",189},
//#line 146 "keyNumbers.hash"
      {"kComma",133},
//#line 203 "keyNumbers.hash"
      {"kHexC",190},
//#line 105 "keyNumbers.hash"
      {"kFour",92},
//#line 62 "keyNumbers.hash"
      {"backSlash",49},
//#line 30 "keyNumbers.hash"
      {"letter_n",17},
//#line 69 "keyNumbers.hash"
      {"forwardSlash",56},
//#line 180 "keyNumbers.hash"
      {"reserved6",167},
//#line 236 "keyNumbers.hash"
      {"reserved16",223},
//#line 112 "keyNumbers.hash"
      {"kDot",99},
//#line 42 "keyNumbers.hash"
      {"letter_z",29},
//#line 36 "keyNumbers.hash"
      {"letter_t",23},
//#line 224 "keyNumbers.hash"
      {"kMemAdd",211},
//#line 217 "keyNumbers.hash"
      {"kHashTag",204},
//#line 120 "keyNumbers.hash"
      {"f16",107},
//#line 13 "keyNumbers.hash"
      {"reserved0",0},
//#line 184 "keyNumbers.hash"
      {"reserved10",171},
//#line 87 "keyNumbers.hash"
      {"home",74},
//#line 64 "keyNumbers.hash"
      {"twoDots",51},
//#line 234 "keyNumbers.hash"
      {"kHexa",221},
//#line 174 "keyNumbers.hash"
      {"Oper",161},
//#line 97 "keyNumbers.hash"
      {"kForwardSlash",84},
//#line 56 "keyNumbers.hash"
      {"tab",43},
//#line 44 "keyNumbers.hash"
      {"n_two",31},
//#line 179 "keyNumbers.hash"
      {"reserved5",166},
//#line 235 "keyNumbers.hash"
      {"reserved15",222},
//#line 204 "keyNumbers.hash"
      {"kHexD",191},
//#line 192 "keyNumbers.hash"
      {"decSep",179},
//#line 171 "keyNumbers.hash"
      {"Return",158},
//#line 108 "keyNumbers.hash"
      {"kSeven",95},
//#line 91 "keyNumbers.hash"
      {"pageDown",78},
//#line 238 "keyNumbers.hash"
      {"lShift",225},
//#line 225 "keyNumbers.hash"
      {"kMemSubtract",212},
//#line 86 "keyNumbers.hash"
      {"insert",73},
//#line 61 "keyNumbers.hash"
      {"cBracket",48},
//#line 45 "keyNumbers.hash"
      {"n_three",32},
//#line 134 "keyNumbers.hash"
      {"again",121},
//#line 43 "keyNumbers.hash"
      {"n_one",30},
//#line 194 "keyNumbers.hash"
      {"CurrencySubUnit",181},
//#line 81 "keyNumbers.hash"
      {"f11",68},
//#line 14 "keyNumbers.hash"
      {"reserved1",1},
//#line 185 "keyNumbers.hash"
      {"reserved11",172},
//#line 68 "keyNumbers.hash"
      {"dot",55},
//#line 67 "keyNumbers.hash"
      {"comma",54},
//#line 47 "keyNumbers.hash"
      {"n_five",34},
//#line 191 "keyNumbers.hash"
      {"thSep",178},
//#line 242 "keyNumbers.hash"
      {"rShift",229},
//#line 15 "keyNumbers.hash"
      {"reserved2",2},
//#line 186 "keyNumbers.hash"
      {"reserved12",173},
//#line 167 "keyNumbers.hash"
      {"SysReq",154},
//#line 173 "keyNumbers.hash"
      {"Out",160},
//#line 138 "keyNumbers.hash"
      {"past",125},
//#line 181 "keyNumbers.hash"
      {"reserved7",168},
//#line 136 "keyNumbers.hash"
      {"cut",123},
//#line 96 "keyNumbers.hash"
      {"numLock",83},
//#line 114 "keyNumbers.hash"
      {"application",101},
//#line 66 "keyNumbers.hash"
      {"acuteAcent",53},
//#line 46 "keyNumbers.hash"
      {"n_four",33},
//#line 48 "keyNumbers.hash"
      {"n_six",35},
//#line 50 "keyNumbers.hash"
      {"n_eight",37},
//#line 49 "keyNumbers.hash"
      {"n_seven",36}
    };

  static short lookup[] =
    {
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
        0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,   1,   2,  -1,  -1,
       -1,   3,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,   4,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,   5,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,   6,  -1,  -1,  -1,   7,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,   8,   9,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  10,
       -1,  11,  -1,  -1,  -1,  12,  13,  -1,  -1,  -1,
       14,  15,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  16,  17,
       -1,  -1,  -1,  -1,  18,  19,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  20,  -1,  -1,  -1,  21,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  22,  -1,  23,  -1,
       -1,  -1,  -1,  24,  25,  -1,  -1,  26,  27,  28,
       29,  30,  -1,  -1,  31,  -1,  32,  33,  34,  -1,
       35,  -1,  36,  37,  -1,  38,  39,  -1,  -1,  40,
       41,  42,  -1,  -1,  43,  -1,  -1,  44,  -1,  -1,
       -1,  -1,  45,  46,  -1,  47,  -1,  48,  -1,  49,
       -1,  50,  51,  52,  -1,  53,  -1,  54,  -1,  -1,
       55,  -1,  56,  -1,  -1,  -1,  57,  -1,  58,  59,
       -1,  60,  -1,  -1,  61,  -1,  -1,  -1,  62,  63,
       64,  -1,  65,  66,  67,  68,  -1,  -1,  -1,  69,
       70,  71,  72,  73,  -1,  74,  -1,  75,  76,  77,
       -1,  -1,  78,  79,  -1,  80,  -1,  81,  82,  -1,
       -1,  83,  -1,  84,  -1,  85,  -1,  86,  87,  -1,
       -1,  88,  -1,  89,  -1,  90,  91,  -1,  -1,  -1,
       92,  93,  -1,  94,  95,  96,  97,  -1,  -1,  -1,
       98,  99,  -1,  -1,  -1, 100,  -1, 101, 102, 103,
       -1, 104, 105, 106, 107, 108, 109, 110, 111, 112,
      113,  -1, 114, 115, 116, 117, 118, 119,  -1, 120,
      121, 122, 123,  -1, 124, 125,  -1, 126, 127,  -1,
      128, 129, 130, 131,  -1, 132, 133, 134,  -1,  -1,
      135,  -1, 136,  -1, 137, 138,  -1, 139,  -1, 140,
      141, 142,  -1, 143, 144, 145, 146, 147, 148, 149,
      150,  -1,  -1, 151, 152, 153,  -1,  -1,  -1,  -1,
       -1, 154,  -1,  -1, 155,  -1,  -1,  -1, 156, 157,
      158, 159,  -1,  -1, 160, 161, 162,  -1, 163, 164,
      165, 166, 167,  -1, 168, 169,  -1, 170, 171, 172,
       -1, 173, 174,  -1, 175, 176, 177,  -1, 178, 179,
       -1,  -1, 180,  -1, 181,  -1,  -1,  -1, 182,  -1,
       -1,  -1,  -1, 183,  -1, 184, 185,  -1,  -1,  -1,
      186, 187,  -1, 188,  -1, 189,  -1,  -1,  -1, 190,
      191,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 192,
       -1, 193,  -1,  -1,  -1, 194,  -1,  -1, 195, 196,
       -1,  -1,  -1,  -1,  -1, 197, 198, 199,  -1, 200,
       -1,  -1,  -1,  -1,  -1, 201,  -1, 202,  -1,  -1,
       -1,  -1, 203, 204, 205,  -1,  -1,  -1,  -1, 206,
      207,  -1, 208,  -1,  -1,  -1, 209,  -1, 210,  -1,
      211, 212,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1, 213,  -1,  -1,  -1,  -1,  -1,
      214,  -1,  -1, 215,  -1,  -1,  -1,  -1, 216,  -1,
       -1,  -1, 217, 218, 219,  -1,  -1,  -1,  -1, 220,
      221,  -1,  -1, 222,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1, 223, 224,  -1,  -1,  -1,  -1,  -1,
      225,  -1,  -1,  -1,  -1, 226,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      227,  -1,  -1,  -1,  -1, 228,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1, 229,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1, 230,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1, 231
    };
std::cout<<"here"<<std::endl;
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
       unsigned int key = lhash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          int index = lookup[key];

          if (index >= 0)
            {
               const char *s = wordlist[index].name;

              if (*str == *s && !strcmp (str + 1, s + 1)){
                  ret = wordlist[index];
              }
                
            }
        }
    }
  return ret;
}

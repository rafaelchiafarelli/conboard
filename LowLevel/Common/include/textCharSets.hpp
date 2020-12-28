#pragma once       
#include <assert.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>

class textCharSet { 
  public:
   const char *cmd; 
   const char *first; 
   const char *second; 
   const char *third; 
   const char *fourth;
   };

#define TEXT_CHAR_SET {{ .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd = "tab", },\
      { .cmd = "enter", },\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd =NULL,NULL,NULL,NULL,NULL},\
      { .cmd = "space", },\
      { .cmd="lShift",	.first =	"n_one",	NULL,NULL,NULL},\
      { .cmd="lAlt",		.first =	"n_three", .second="n_four",NULL,NULL},\
      { .cmd="lShift",	.first =	"n_three",NULL,NULL,NULL},\
      { .cmd="lShift",	.first =	"n_four",	NULL,NULL,NULL},\
      { .cmd="lShift", 	.first =	"n_five",	NULL,NULL,NULL},\
      { .cmd="lShift", 	.first =	"n_seven",	NULL,NULL,NULL},\
      { .cmd="singleCuotes",	NULL,NULL,NULL,NULL},\
      { .cmd="lShift", .first="n_nine",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="n_zero",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="n_eight",	NULL,NULL,NULL},\
      { .cmd="kPlus",	NULL,NULL,NULL,NULL},\
      { .cmd="comma",	NULL,NULL,NULL,NULL},\
      { .cmd="minus",	NULL,NULL,NULL,NULL},\
      { .cmd="dot",	NULL,NULL,NULL,NULL},\
      { .cmd="forwardSlash",	NULL,NULL,NULL,NULL},\
      { .cmd="n_zero",	NULL,NULL,NULL,NULL},\
      { .cmd="n_one",	NULL,NULL,NULL,NULL},\
      { .cmd="n_two",	NULL,NULL,NULL,NULL},\
      { .cmd="n_three",	NULL,NULL,NULL,NULL},\
      { .cmd="n_four",	NULL,NULL,NULL,NULL},\
      { .cmd="n_five",	NULL,NULL,NULL,NULL},\
      { .cmd="n_six",	NULL,NULL,NULL,NULL},\
      { .cmd="n_seven",	NULL,NULL,NULL,NULL},\
      { .cmd="n_eight",	NULL,NULL,NULL,NULL},\
      { .cmd="n_nine",	NULL,NULL,NULL,NULL},\
      { .cmd="twoDots",	NULL,NULL,NULL,NULL},\
      { .cmd="lShift", .first="twoDots",NULL,NULL,NULL},\
      { .cmd="lShift", .first="comma",NULL,NULL,NULL},\
      { .cmd="equal",	NULL,NULL,NULL,NULL},\
      { .cmd="lShift", .first="dot",NULL,NULL,NULL},\
      { .cmd="lShift", .first="forwardSlash",NULL,NULL,NULL},\
      { .cmd="lShift", .first="n_two",NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_a",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_b",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_c",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_d",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_e",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_f",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_g",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_h",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_i",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_j",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_k",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_l",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_m",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_n",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_o",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_p",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_q",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_r",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_s",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_t",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_u",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_v",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_w",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_x",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_y",	NULL,NULL,NULL},\
      { .cmd="lShift", .first="letter_z",	NULL,NULL,NULL},\
      { .cmd="oBracket",	NULL,NULL,NULL,NULL},\
      { .cmd="backSlash",NULL,NULL,NULL,NULL},\
      { .cmd="cBracket",	NULL,NULL,NULL,NULL},\
      { .cmd="lShift", .first="Tilde",NULL,NULL,NULL},\
      { .cmd="lShift", .first="minus",NULL,NULL,NULL},\
      { .cmd="lShift", .first="acuteAcent",NULL,NULL,NULL},\
      { .cmd="letter_a",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_b",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_c",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_d",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_e",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_f",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_g",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_h",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_i",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_j",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_k",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_l",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_m",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_n",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_o",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_p",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_q",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_r",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_s",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_t",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_u",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_v",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_w",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_x",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_y",	NULL,NULL,NULL,NULL},\
      { .cmd="letter_z",	NULL,NULL,NULL,NULL},\
      { .cmd="lShift", .first="oBracket",NULL,NULL,NULL},\
      { .cmd="lShift", .first="backSlash",NULL,NULL,NULL},\
      { .cmd="lShift", .first="cBracket",NULL,NULL,NULL},\
      { .cmd="Tilde",NULL,NULL,NULL,NULL},\
      {.cmd =NULL,NULL,NULL,NULL,NULL},\
      {.cmd = "lAlt", .first="n_one",.second="n_two", .third="n_eight",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_two", .third="n_nine",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_three",.third="n_zero",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_three",.third="n_one",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_three",.third="n_two",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_three",.third="n_three",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_three",.third="n_four",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_three",.third="n_five",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_three",.third="n_six",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_three",.third="n_seven",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_three",.third="n_eight",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_three",.third="n_nine",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_four",.third="n_zero",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_four",.third="n_one",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_four",.third="n_five",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_four",.third="n_six",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_four",.third="n_seven",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_four",.third="n_eight",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_four",.third="n_nine",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_five",.third="n_zero",NULL},\
      { .cmd="lAlt",    "n_one", 	 .second="n_five",.third="n_one",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_five",.third="n_two",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_five",.third="n_five",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_five",.third="n_six",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_five",.third="n_seven",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_five",.third="n_eight",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_five",.third="n_nine",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_six", .third="n_zero",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_six", .third="n_one",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_six", .third="n_two",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_six", .third="n_three",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_six", .third="n_four",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_six", .third="n_five",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_six", .third="n_six",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_six", .third="n_seven",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_six", .third="n_eight",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_seven",.third="n_zero",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_seven",.third="n_one",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_seven",.third="n_two",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_seven",.third="n_three",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_seven",.third="n_four",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_seven",.third="n_five",NULL},\
      { .cmd="lAlt", .first="n_one", .second="n_seven",.third="n_nine",NULL},\
      { .cmd="lAlt", .first="n_two", .second="n_two", .third="n_five",NULL},\
      { .cmd="lAlt", .first="n_two", .second="n_three",.third="n_zero",NULL},\
      { .cmd="lAlt", .first="n_two", .second="n_four",.third="n_one",NULL},\
      { .cmd="lAlt", .first="n_two", .second="n_four",.third="n_eight",NULL},\
      { .cmd="lAlt", .first="n_two", .second="n_four",.third="n_nine",NULL},\
      { .cmd="lAlt", .first="n_two", .second="n_five",.third="n_zero",NULL},\
      { .cmd="lAlt", .first="n_two", .second="n_five",.third="n_three",NULL},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_two",  .fourth="n_eight"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_three",.fourth="n_two"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_three",.fourth="n_three"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_three",.fourth="n_four"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_three",.fourth="n_five"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_three",.fourth="n_six"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_three",.fourth="n_seven"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_three",.fourth="n_eight"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_three",.fourth="n_nine"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_four", .fourth="n_zero"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_four", .fourth="n_five"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_four", .fourth="n_six"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_four", .fourth="n_seven"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_four", .fourth="n_eight"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_five", .fourth="n_zero"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_five", .fourth="n_one"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_five", .fourth="n_two"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_five", .fourth="n_three"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_five", .fourth="n_four"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_five", .fourth="n_five"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_five", .fourth="n_six"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_five", .fourth="n_nine"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_six",  .fourth="n_eight"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_six",  .fourth="n_nine"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_seven",.fourth="n_four"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_seven",.fourth="n_five"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_seven",.fourth="n_nine"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_eight",.fourth="n_zero"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_eight",.fourth="n_four"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_eight",.fourth="n_five"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_nine", .fourth="n_zero"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_nine", .fourth="n_two"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_nine", .fourth="n_three"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_nine", .fourth="n_four"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_nine", .fourth="n_five"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_nine", .fourth="n_six"},\
      { .cmd="lAlt", .first="n_zero",.second="n_one", .third="n_nine", .fourth="n_seven"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_zero", .fourth="n_zero"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_zero", .fourth="n_one"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_zero", .fourth="n_two"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_zero", .fourth="n_three"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_zero", .fourth="n_four"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_zero", .fourth="n_five"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_zero", .fourth="n_six"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_zero", .fourth="n_seven"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_zero", .fourth="n_eight"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_one",  .fourth="n_zero"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_one",  .fourth="n_one"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_one",  .fourth="n_two"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_one",  .fourth="n_three"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_one",  .fourth="n_four"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_one",  .fourth="n_five"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_one",  .fourth="n_six"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_one",  .fourth="n_seven"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_one",  .fourth="n_eight"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_one",  .fourth="n_nine"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_two",  .fourth="n_zero"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_two",  .fourth="n_one"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_two",  .fourth="n_two"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_two",  .fourth="n_seven"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_four", .fourth="n_zero"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_four", .fourth="n_five"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_four", .fourth="n_seven"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_four", .fourth="n_eight"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_five", .fourth="n_two"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_five", .fourth="n_three"},\
      { .cmd="lAlt", .first="n_zero",.second="n_two", .third="n_five", .fourth="n_four"}};

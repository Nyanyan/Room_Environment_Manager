# include <SoftwareSerial.h>

# define swrxPin 32
# define swtxPin 33
SoftwareSerial Printer;

#define N_CHARS_PER_LINE 32

/*
#define N_LINES 20
char ascii_art[N_LINES][N_CHARS_PER_LINE] = {
"        ,-=^'\"\"'^=-,",
"      =`>`:::::::::: `=",
"    /\" /    ::::::     \"\\",
"   /  /       ::::       \\,",
"  ,^ |    /````/`/^\\     ^,",
"  | |    |..::| |   |     |",
"  ', | .::\\::::\\_\\./:::  ,'",
"   \\ \\::::|::: |  :::::: /",
"    \\ -:::|::  |  :::: ./",
"     `=_`:|:   |  :`_=`",
"        `-=.,__,.=-`",
"          |    |",
"          |    |",
"          |    |",
"          |    |",
"          |    |",
"          |    |",
"    ,.-r^^/,__,\\^^r-.,",
"   (    \\.,____,./    )",
"    `--..,,_____,,..-''"
};
*/

#define N_LINES 16
char ascii_art[N_LINES][N_CHARS_PER_LINE] = {
"    ..,z&a-.,?T9ma+...",
"  .H#NG{_HFM# ,* &%A\\HHHm,.",
".JNMMNMMQeDA .... #J.dMHHHN,",
"jNMNMMNMHN ......, HNMHHNTMHH|",
"XNgaqMMMM ........, NN<~_~(JMH#",
"jMNMHMNNH -------. NMMNdHHHHH$",
".WTTMMMW |-------:  WHMMMHQH@",
".`<i_?Cww `-----` MHHMkMgMM",
"  `.7i.-TMHXww0OVCzOWMMg",
"       ?T61+.......-`",
"       .kI/",
"      .ZO!",
"      dO!",
"     dO:",
"    JI:",
"   ?Wt"
};

/*

        ,-=^'""'^=-,
      =`>`:::::::::: `=
    /" /    ::::::     "\
   /  /       ::::       \,
  ,^ |    /````/`/^\     ^,
  | |    |..::| |   |     |
  ', | .::\::::\_\./:::  ,'
   \ \::::|::: |  :::::: /
    \ -:::|::  |  :::: ./
     `=_`:|:   |  :`_=`
        `-=.,__,.=-`
          |    |
          |    |
          |    |
          |    |
          |    |
          |    |
    ,.-r^^/,__,\^^r-.,
   (    \.,____,./    )
    `--..,,_____,,..-''


*/




/*
    ..,z&a-.,?T9ma+...
  .H#NG{_HFM# ,* &%A\HHHm,.
.JNMMNMMQeDA .... #J.dMHHHN,
jNMNMMNMHN ......, HNMHHNTMHH|
XNgaqMMMM ........, NN<~_~(JMH#
jMNMHMNNH -------. NMMNdHHHHH$
.WTTMMMW |-------:  WHMMMHQH@
.`<i_?Cww `-----` MHHMkMgMM
  `.7i.-TMHXww0OVCzOWMMg
       ?T61+.......-`
       .kI/
      .ZO!
      dO!
     dO:
    JI:
   ?Wt

*/

void setup() {

  pinMode(swrxPin, INPUT);
  pinMode(swtxPin, OUTPUT);
  Printer.begin(9600, SWSERIAL_8N1, swrxPin, swtxPin , false, 256);

  // Serial.begin(9600);
  
  // Serial.println("The quick brown fox jumps over the lazy dog.");
  // Serial.write(10); //Send LF
  // Serial.println("Even pig will try to climb a tree if you flatter him.");
  // Serial.write(10);
  // Serial.write(10);

  Printer.write(10);
  for (int i = 0; i < N_LINES; ++i) {
    Printer.println(ascii_art[i]);
    // Printer.write(10);
  }
  Printer.write(10);
  Printer.write(10);
}

void loop()
{
}
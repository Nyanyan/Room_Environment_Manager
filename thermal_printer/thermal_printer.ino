
#define N_LINES 20
#define N_CHARS_PER_LINE 32

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

void setup() {
  Serial.begin(9600);
  
  // Serial.println("The quick brown fox jumps over the lazy dog.");
  // Serial.write(10); //Send LF
  // Serial.println("Even pig will try to climb a tree if you flatter him.");
  // Serial.write(10);
  // Serial.write(10);

  for (int i = 0; i < N_LINES; ++i) {
    Serial.println(ascii_art[i]);
    // Serial.write(10);
  }
  Serial.write(10);
  Serial.write(10);
}

void loop()
{
}
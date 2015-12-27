# Enigma

   Enigma.cpp

   Enigma Simulator by James A. McCombe, 2013
   
   Small tribute to Alan Touring after an inspiring visit to the amazing 
   Bletchly Park museum in Milton Keynes.
   This is a quick hack to simulate an Enigma machine used during World War II.
   
   I highly recommend this webpage to learn about the mechanics of the Enigma 
   machine that is being simulated here:
   
   http://users.telenet.be/d.rijmenants/en/enigmatech.htm
   
   COMPILATION INSTRUCTIONS:
   
   ( tested compilation on Linux and Mac OS X only )

   gcc -Wall -Werror Enigma.cpp -o Enigma

   SCARY EXAMPLE:

   Enigma Instruction Manual, 1930:
   Commandline : ./Enigma -r 213 -rs XMV -sp ABL -s AM,FI,NV,PS,TU,WZ -rf A -q
   Settings    : Reflector A, Wheels II,I,III, Ringstellung 24,13,22, Steckers AM,FI,NV,PS,TU,WZ
   Ciphertext  : GCDSE AHUGW TQGRK VLFGX UCALX VYMIG MMNMF DXTGN VHVRM MEVOU YFZSL RHDRR XFJWC FHUHM UNZEF RDISI KBGPM YVXUZ
   Decrypt     : FEIND LIQEI NFANT ERIEK OLONN EBEOB AQTET XANFA NGSUE DAUSG ANGBA ERWAL DEXEN DEDRE IKMOS TWAER TSNEU STADT
   German      : Feindliche Infanterie Kolonne beobachtet. Anfang Südausgang Bärwalde. Ende 3km ostwärts Neustadt.
   English     : Enemy infantry column was observed. Beginning [at] southern exit [of] Baerwalde. Ending 3km east of Neustadt.

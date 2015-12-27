/* 
   ================================================================================
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
   ================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define WALZE_COUNT        3

const char *walze_I_config         = "EKMFLGDQVZNTOWYHXUSPAIBRCJ";
const char  walze_I_notch          = 'Q';
const char *walze_II_config        = "AJDKSIRUXBLHWTMCQGZNPYFVOE";
const char  walze_II_notch         = 'E';
const char *walze_III_config       = "BDFHJLCPRTXVZNYEIWGAKMUSQO";
const char  walze_III_notch        = 'V';
const char *walze_IV_config        = "ESOVPZJAYQUIRHXLNFTGKDCMWB";
const char  walze_IV_notch         = 'J';
const char *walze_V_config         = "VZBRGITYUPSDNHLXAWMJQOFECK";
const char  walze_V_notch          = 'Z';
const char *umkehrwalze_A_config   = "EJMZALYXVBWFCRQUONTSPIKHGD";
const char *umkehrwalze_B_config   = "YRUHQSLDPXNGOKMIEBFZCWVJAT";

typedef struct _WalzeState {
	unsigned char config             [26];  /* Configuration array for the wheel.  Only used to pretty print the wheel */
	unsigned char config_reciprocal  [26];  /* Reciprocal of wheel configuration array.  Used for reflecting signal in 
											   the reverse direction through the wheels                                */
	unsigned char notch;                    /* Notch position.  When position == notch, it rotates the next wheel      */
	unsigned char position;                 /* Wheel position.  This is the wheel character shown through the window   */
	unsigned char ringstellung;             /* Alphabet ring position.                                                 */
	
	/* Retain the character at each stage of the wheel so we can pretty print the machine state later */
	struct {                                
		struct {
			unsigned char input;
			unsigned char wires_input;
			unsigned char wires_output;
			unsigned char output;
		} forward, reciprocal;
	} last_character_debug;
} WalzeState;

typedef struct _EnigmaState {
	WalzeState    walze          [WALZE_COUNT];  /* State of each wheel installed in the Enigma           */
	unsigned char umkehrwalze             [26];  /* Umkehrwalze "Reversal Rotor" permute                  */
	unsigned char steckerboard            [26];  /* Steckerboard "Plug Board" permute                     */
	unsigned char steckerboard_reciprocal [26];  /* Reciprocal of Steckerboard "Plug Board" permute       */
	bool          enable_debug_output;

	/* Retain the character at each stage of the machine so we can pretty print the machine state later */
	struct {
		unsigned char steckerboard_forward_input;
		unsigned char steckerboard_forward_output;
		unsigned char umkehrwalze_input;
		unsigned char umkehrwalze_output;
		unsigned char steckerboard_reciprocal_input;
		unsigned char steckerboard_reciprocal_output;
	} last_character_debug;
} EnigmaState;

/* ================================================================================ */
/* Utility routines                                                                 */
void PrintWires(unsigned char wire_start, unsigned char wire_end)
{
	if (wire_end < wire_start) {
		unsigned char t = wire_start;
		wire_start      = wire_end;
		wire_end        = t;
	}
	
	for (unsigned int i = 0; i < 26; i ++) {
		if ((i == wire_start) || (i == wire_end))
			putchar('+');
		else if ((i > wire_start) && (i < wire_end))
			putchar('-');
		else
			putchar(' ');
	}
}

void PrintAlphabet(unsigned char offset, unsigned char highlight_0, unsigned char highlight_1)
{
	for (unsigned int i = 0; i < 26; i ++) {
		unsigned int n = ((i + offset) % 26);
		if (n == highlight_0) printf("\033[32m");
		if (n == highlight_1) printf("\033[33m");
		putchar('a' + n);
		printf("\033[0m");
	}	
}

void GenerateReciprocalArray(unsigned char input_array [26], unsigned char output_array [26])
{
	for (unsigned int i = 0; i < 26; i ++) 
		output_array[input_array[i]] = i;
}

bool GetCharacter(char *character)
{
	while (1) {
		/* Read the character from the stdin */
		char c = getchar();

		/* Convert UPPERCASE to lowercase */
		/* Remap 'a-z' -> 0-25 */
		if ((c >= 'a') && (c <= 'z')) {
			c -= 'a';
		} else if ((c >= 'A') && (c <= 'Z')) {
			c -= 'A';
		} else {
			switch (c) {
			case -1:  /* EOF */
				return true; 
			default:  /* Skip unknown characters */
				continue;
			}
		}
		*character = 'A' + c;
		break;
	}

	return false;
}
/* ================================================================================ */

/* ================================================================================ */
/* Enigma Wheel routines                                                            */
unsigned char WheelFeedCharacter(WalzeState *walze, bool reciprocal, unsigned char input)
{
	unsigned char output;

	if (reciprocal) {
		walze->last_character_debug.reciprocal.input        = input;
		walze->last_character_debug.reciprocal.wires_input  = ((26 + input + walze->position - walze->ringstellung) % 26);
		walze->last_character_debug.reciprocal.wires_output = walze->config_reciprocal[walze->last_character_debug.reciprocal.wires_input];
		walze->last_character_debug.reciprocal.output       = ((26 + walze->last_character_debug.reciprocal.wires_output - walze->position + walze->ringstellung) % 26);

		output = walze->last_character_debug.reciprocal.output;
	} else {
		walze->last_character_debug.forward.input        = input;
		walze->last_character_debug.forward.wires_input  = ((26 + input + walze->position - walze->ringstellung) % 26);
		walze->last_character_debug.forward.wires_output = walze->config[walze->last_character_debug.forward.wires_input];
		walze->last_character_debug.forward.output       = ((26 + walze->last_character_debug.forward.wires_output - walze->position + walze->ringstellung) % 26);

		output = walze->last_character_debug.forward.output;
	}
	
	return output;
}

void WheelRotate(WalzeState *walze, unsigned int steps)
{
	walze->position = ((walze->position + steps) % 26);	
}

void WheelDebugPrint(WalzeState *walze, unsigned int wheel_index)
{
	printf("\033[44m|\033[0m------------------------- : Walze %i\n", wheel_index);
	PrintAlphabet(0, walze->last_character_debug.forward.input, walze->last_character_debug.reciprocal.output); printf("\n");

	PrintAlphabet(walze->position, walze->last_character_debug.forward.wires_input, walze->last_character_debug.reciprocal.wires_output); printf("\n");

	printf("\033[32m");	
	PrintWires(((walze->last_character_debug.forward.wires_input  + 26) - walze->position) % 26,
			   ((walze->last_character_debug.forward.wires_output + 26) - walze->position) % 26);
	printf("\033[0m\n");

	printf("\033[33m");	
	PrintWires(((walze->last_character_debug.reciprocal.wires_input  + 26) - walze->position) % 26,
			   ((walze->last_character_debug.reciprocal.wires_output + 26) - walze->position) % 26);
	printf("\033[0m\n");

	PrintAlphabet(walze->position, walze->last_character_debug.forward.wires_output, walze->last_character_debug.reciprocal.wires_input); printf("\n");	

	for (unsigned int i = 0; i < 26; i ++) {
		if (i == 0) printf("\033[41m");		
		putchar('A' + ((i + walze->position + walze->ringstellung) % 26));
		if (i == 0) printf("\033[0m");
	}
	printf(" <- Ring Characters\n");

	PrintAlphabet(0, walze->last_character_debug.forward.output, walze->last_character_debug.reciprocal.input);	printf("\n");	
	
	printf("\033[44m|\033[0m-------------------------\n");	
}

void WheelSetConfiguration(WalzeState *walze,
						   const char *config_array, /* Substitution array for the wheel                       */
						   char        notch)        /* Character which trigger next wheel to rotate           */
{
	for (unsigned int i = 0; i < 26; i ++)
		walze->config[i]  = (config_array[i] - 'A');
	GenerateReciprocalArray(walze->config, walze->config_reciprocal);
	walze->notch = (notch - 'A');
}

void WheelSetPosition(WalzeState *walze,
					  char        start_position)    /* 'A'-'Z' starting position for the wheel                */
{
	walze->position = (start_position - 'A');
}

void WheelSetRingstellung(WalzeState *walze,
						  char        ringstellung)  /* 'A'-'Z' Ringstellung (alphabet ring rotation)          */
{
	walze->ringstellung = (ringstellung - 'A');
}
/* ================================================================================ */

/* ================================================================================ */
/* Enigma machine routines                                                          */
char EnigmaFeedCharacter(EnigmaState *enigma, char input_character)
{
	unsigned char temp_character;

	/* Convert the character from range 'A' -> 'Z' to 0 -> 25 */
	temp_character = (input_character - 'A');

	/* Run the character through the steckerboard */
	enigma->last_character_debug.steckerboard_forward_input  = temp_character;	
	enigma->last_character_debug.steckerboard_forward_output = temp_character = enigma->steckerboard[temp_character];
	
	/* Rotate the wheels.  First wheel always rotates or we hit the notch */
	for (unsigned int i = 0; i < WALZE_COUNT; i ++) {
		if ((i == 0) || (enigma->walze[i - 1].position == enigma->walze[i - 1].notch))
			WheelRotate(&enigma->walze[i], 1);
	}

	/* Run the character through each of the wheels */
	for (unsigned int i = 0; i < WALZE_COUNT; i ++) {
		temp_character = WheelFeedCharacter(&enigma->walze[i], false, temp_character);
	}
	
	/* Run it through the Umkehrwalze */
	enigma->last_character_debug.umkehrwalze_input  = temp_character;
	enigma->last_character_debug.umkehrwalze_output = temp_character = enigma->umkehrwalze[temp_character];

	/* Run it back through the wheels in reverse order */
	for (unsigned int i = 0; i < WALZE_COUNT; i ++) {
		temp_character = WheelFeedCharacter(&enigma->walze[WALZE_COUNT - i - 1], true, temp_character);
	}

	/* Run it through the steckerboard in reverse order */
	enigma->last_character_debug.steckerboard_reciprocal_input  = temp_character;
	enigma->last_character_debug.steckerboard_reciprocal_output = temp_character = enigma->steckerboard_reciprocal[temp_character];
	
	/* Convert back to 'A' - 'Z' */
	temp_character += 'A';
	
	/* Debug output */
	if (enigma->enable_debug_output) {
		printf("Input Character : %c;\n", input_character);
		printf("===============================================\n");
		PrintAlphabet(0, enigma->last_character_debug.steckerboard_forward_input, enigma->last_character_debug.steckerboard_reciprocal_output); printf(" : Steckerboard\n");
		printf("\033[32m"); PrintWires(enigma->last_character_debug.steckerboard_forward_input, enigma->last_character_debug.steckerboard_forward_output);          printf("\n");
		printf("\033[33m"); PrintWires(enigma->last_character_debug.steckerboard_reciprocal_input, enigma->last_character_debug.steckerboard_reciprocal_output);    printf("\n");
		printf("\033[0m");  PrintAlphabet(0, enigma->last_character_debug.steckerboard_forward_output, enigma->last_character_debug.steckerboard_reciprocal_input); printf("\n");
		printf("===============================================\n");
		
		for (unsigned int i = 0; i < WALZE_COUNT; i ++)
			WheelDebugPrint(&enigma->walze[i], i);

		printf("===============================================\n");		
		PrintAlphabet(0, enigma->last_character_debug.umkehrwalze_input, enigma->last_character_debug.umkehrwalze_output); printf(" : Umkehrwalze\n");
		PrintWires(enigma->last_character_debug.umkehrwalze_input, enigma->last_character_debug.umkehrwalze_output);       printf("\n");
		printf("===============================================\n");
		printf("Output Character : %c;\n\n\n", temp_character);
	} else {
		putchar(temp_character);
	}

	return temp_character;
}

void EnigmaSetUmkehrwalze(EnigmaState *enigma, 
						  const char   config_array [26])
{
	for (unsigned int i = 0; i < 26; i ++)
		enigma->umkehrwalze[i] = (config_array[i] - 'A');
}

void EnigmaSetSteckerboard(EnigmaState *enigma, 
						   const char   config_array [26])
{
	for (unsigned int i = 0; i < 26; i ++)
		enigma->steckerboard[i] = (config_array[i] - 'A');
	GenerateReciprocalArray(enigma->steckerboard, enigma->steckerboard_reciprocal);
}
/* ================================================================================ */

void DisplayUsageInformation(char *name)
{
	printf("%s [options]\n\n", name);
	printf("================================================================================\n");
	printf("Enigma Simulator by James A. McCombe, 2013\n\n");
	printf("Small tribute to Alan Touring after an inspiring visit to the amazing\n");
	printf("Bletchly Park museum in Milton Keynes.\n");
	printf("This is a quick hack to simulate an Enigma machine used during World War II.\n\n");
	printf("Defaults: 3 Rotors, No steckers, Ringstellung=AAA, StartPosition=AAA\n");
	printf("          Rotor 0 (left)  = Type I,   1930, Enigma I\n");
	printf("          Rotor 1         = Type II,  1930, Enigma I\n");
	printf("          Rotor 2 (right) = Type III, 1930, Enigma I\n");
	printf("          Reflector       = Type B (wide)\n\n");
	printf("Rotor Codes:\n");
	printf("          1 = Type I,   1930, Enigma I\n");
	printf("          2 = Type II,  1930, Enigma I\n");
	printf("          3 = Type III, 1930, Enigma I\n");
	printf("          4 = Type IV,  Dec. 1938, M3 Army\n");
	printf("          5 = Type V,   Dec. 1938, M3 Army\n\n");
	printf("Reflector Codes:\n");
	printf("          A = Type A,      Enigma I/M3\n");
	printf("          B = Type Wide-B, Enigma I/M3\n");
	printf("Command line options:\n");
	printf(" [-s    | --steckerboard   ] : AZ,TU will flip A-Z and T-U]\n");
	printf(" [-r    | --rotor          ] : Specified left to right, e.g. 123 is the default config]:\n");
	printf(" [-rs   | --ringstellung   ] : Specified left to left, e.g. AAA or 000 is default config\n"); 
	printf(" [-sp   | --startposition  ] : Specified left to left, e.g. AAA or 000 is default config\n");
	printf(" [-rf   | --reflector      ] : e.g. B;\n");
	printf(" [-q    | --quiet          ] : Only output the ciphertext, no machine visualization.  Handy for piping to files\n");
	printf("\n\n");
}

void ProcessCmdLineArguments(int argc, char **argv, EnigmaState *enigma)
{
	int i;

	for (i = 1; i < argc;) {
		if ((!strcmp(argv[i], "-q")) || (!strcmp(argv[i], "--quiet"))) {
			enigma->enable_debug_output = false;
		} else if ((!strcmp(argv[i], "-s")) || (!strcmp(argv[i], "--steckerboard"))) {
			char         *string          = argv[++ i];
			unsigned int  length          = strlen(string);
			char          steckerboard [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
			if ((length + 1) % 3) {
				printf("Invalid steckerboard configuration\n");
				goto bail;
			}
			for (unsigned int j = 0; j < length; j ++) {
				if ((string[j] < 'A') || (string[j] > 'Z')) {
					printf("Invalid steckerboard configuration character %c.  Must be A-Z\n", string[j]);
					goto bail;
				}
				steckerboard[string[j    ] - 'A'] = string[j + 1];
				steckerboard[string[j + 1] - 'A'] = string[j];
				j += 2;
			}
			EnigmaSetSteckerboard(enigma, steckerboard);
		} else if ((!strcmp(argv[i], "-r")) || (!strcmp(argv[i], "--rotor"))) {
			char         *string = argv[++ i];
			unsigned int  length = strlen(string);
			if (length != 3) {
				printf("Invalid rotor configuration\n");
				goto bail;
			}
			for (unsigned int j = 0; j < length; j ++) {
				WalzeState *walze = &enigma->walze[length - j - 1];
				switch (string[j]) {
				case '1': WheelSetConfiguration(walze, walze_I_config,   walze_I_notch);   break;
				case '2': WheelSetConfiguration(walze, walze_II_config,  walze_II_notch);  break;
				case '3': WheelSetConfiguration(walze, walze_III_config, walze_III_notch); break;
				case '4': WheelSetConfiguration(walze, walze_IV_config,  walze_IV_notch);  break;
				case '5': WheelSetConfiguration(walze, walze_V_config,   walze_V_notch);   break;
				default:
					printf("Invalid rotor type %c.  Must be 1-5\n", string[j]);
					goto bail;
				}
			}
		} else if ((!strcmp(argv[i], "-rs")) || (!strcmp(argv[i], "--ringstellung"))) {
			char         *string = argv[++ i];
			unsigned int  length = strlen(string);
			if (length != 3) {
				printf("Invalid ringstellung configuration\n");
				goto bail;
			}
			for (unsigned int j = 0; j < length; j ++) {
				if ((string[j] < 'A') || (string[j] > 'Z')) {
					printf("Invalid ringstellung configuration character %c.  Must be A-Z\n", string[j]);
					goto bail;
				}				
				WheelSetRingstellung(&enigma->walze[length - j - 1], string[j]);
			}
		} else if ((!strcmp(argv[i], "-sp")) || (!strcmp(argv[i], "--startposition"))) {
			char         *string = argv[++ i];
			unsigned int  length = strlen(string);
			if (length != 3) {
				printf("Invalid rotor start position configuration\n");
				goto bail;
			}
			for (unsigned int j = 0; j < length; j ++) {
				if ((string[j] < 'A') || (string[j] > 'Z')) {
					printf("Invalid rotor start position configuration character %c.  Must be A-Z\n", string[j]);
					goto bail;
				}				
				WheelSetPosition(&enigma->walze[length - j - 1], string[j]);
			}
		} else if ((!strcmp(argv[i], "-rf")) || (!strcmp(argv[i], "--reflector"))) {
			char         *string = argv[++ i];
			unsigned int  length = strlen(string);
			if (length != 1) {
				printf("Invalid reflector configuration\n");
				goto bail;
			}
			for (unsigned int j = 0; j < length; j ++) {
				switch (string[j]) {
				case 'A': EnigmaSetUmkehrwalze(enigma, umkehrwalze_A_config); break;
				case 'B': EnigmaSetUmkehrwalze(enigma, umkehrwalze_B_config); break;
				default: 
					printf("Invalid reflector type %c.  Must be A or B\n", string[j]);
					goto bail;
				}				
			}
		} else {
			printf("Unknown option\n");
			goto bail;
		}
		i ++;
	}

	if (enigma->enable_debug_output)
		DisplayUsageInformation(argv[0]);
	return;

 bail:
	exit(1);
}

int main(int argc, char **argv)
{
	EnigmaState enigma;

	/* Setup enigma machine defaults */
	memset(&enigma, 0, sizeof(EnigmaState));
	enigma.enable_debug_output = true;
	WheelSetConfiguration(&enigma.walze[0], walze_III_config, walze_III_notch);
	WheelSetConfiguration(&enigma.walze[1], walze_II_config,  walze_II_notch);
	WheelSetConfiguration(&enigma.walze[2], walze_I_config,   walze_I_notch);	
	EnigmaSetUmkehrwalze(&enigma,  umkehrwalze_B_config);
	EnigmaSetSteckerboard(&enigma, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

	/* Process command line arguments */
	ProcessCmdLineArguments(argc, argv, &enigma);	
	
	{
		char character;
		
		while (!GetCharacter(&character))
			EnigmaFeedCharacter(&enigma, character);
	}
	
	return 0;
}

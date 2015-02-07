int TRUE = 1;
int FALSE = 0;

//Circular Que creation
byte buffer[4][9]; //4 cells buffer
int front = 0; //front of que
int rear = 0; //end of que
int cellsUsed = 0; //used in checking to determine number of cells used per character
int buffer_len = 0; //how many elements in que
int charReturned; //ascii value of character 
int queEmpty = TRUE; //whether or not que is empty; used because front = rear when full 

byte arrToCheck[4][9]; //in order array to be created from buffer

//functions
int allUnpressed(int buttonsPressed[9]);

void setup()
{
  int i = 0;
  Serial.begin(9600);
  for (i=2; i<=10; i++) //don't use pin 0 or 1 due to arduino set up issues
  {
    pinMode(i, INPUT); //initializes pins
  }
}

void loop ()
{
  byte buttonsPressed[9] = {0,0,0,0,0,0,0,0,0}; //resets to zero each loop because new button to be read
  byte brailleCell[9] = {0,0,0,0,0,0,0,0,0}; //same as above
  int unpressedCheck = FALSE; //buttons have not just been upressed
  int i;

  
  //reads buttons pressed when all buttons released
  while (unpressedCheck != TRUE)
  {
    for (i = 0; i <= 8; i++) //check all 9 buttons
    {
      if (brailleCell[i] == 0) //Do not change stored button input from pressed to unpressed until after input has been determined
      {
        brailleCell[i] = digitalRead(i+2); //stores all buttons pressed during one input
      }
      buttonsPressed[i] = digitalRead(i+2); //stores the current buttons pressed
    }
    unpressedCheck = allUnpressed(buttonsPressed); //checks if current state of buttons are unpressed, if so the buttons pressed are in brailleCell
  }
  //read value into buffer
  if (allUnpressed(brailleCell) == 0) //Some button(s) were pressed
  {
    //delay(150); // quick fix to prevent multiple button reads, look into better keyboard
    buffer_len = lenQue(front, rear, queEmpty); //how many braille cells in buffer
    if (brailleCell[7] == 1) //enter key pressed, buffer to be cleared
    {
      while (buffer_len > 0) //will enter loop if still braille input in buffer
      {
        queToArray(buffer, &front, arrToCheck, buffer_len); //Orders inputs. arrToChecks first element will be the oldest input and last element will be most recent
        charReturned = checkCharacter(arrToCheck, buffer_len, &cellsUsed); //checks the inputs and determines character
        if (charReturned == -1) //unable to translate
        {
          Serial.print(" ? "); //alert user of bad entry
          cellsUsed = 1; //will move cursor over to remove bad input
        }
        buffer_len = buffer_len - cellsUsed; //updates length to account for how many cells the determined character was
        updateQue(&front, &rear, &cellsUsed, &queEmpty); //updates buffer to remove printed characters braille combinations
        if (charReturned != -1) //only print valid characters
        { 
          Serial.print((char)charReturned); //printed character, send via bluetooth
        }
      }
      Serial.print("\n"); //enter printed for the input of enter, send via bluetooth
      enter_updateQue(&front, &rear, &queEmpty); //resets buffer to empty
    }
    else
    {
      bufferAdd(brailleCell, buffer, &rear, &queEmpty); //adds value to current and updates current values
      buffer_len = lenQue(front, rear, queEmpty); //number of inputs determined
      if (buffer_len == 4) //buffer is full
      {
        queToArray(buffer, &front, arrToCheck, buffer_len); //Orders inputs. arrToChecks first element will be the oldest input and last element will be most recent
        charReturned = checkCharacter(arrToCheck, buffer_len, &cellsUsed); //determines character that was input
        if (charReturned == -1) //entry could not be translated
        {
          Serial.print(" ? "); //alert user of unknown input
          cellsUsed = 1; //will iterate past unknown input
        }
        updateQue(&front, &rear, &cellsUsed, &queEmpty); //Updates buffer to account for character returned
        if (charReturned != -1) //only print valid characters
        {
          Serial.print((char)charReturned); //character to be printed, send via bluetooth
        }
      }
    }
  }
}

//---------------------button checking functions--------------------------------
//returns True if all buttons unpressed, False if at least one button is pressed
int allUnpressed(byte buttonsPressed[9])
{
  int i;
  for (i = 0; i < 9; i++) //9 buttons to check
  {
    if (buttonsPressed[i] == 1) //button is pressed if 1
    {
      return FALSE; //returned because button is pressed
    }
  }
  return TRUE; //loop went through 9 times, no button pressed
}

//------------------------Buffer manipulation functions------------------------
//Increments rear and adds the input, brailleCell to that index
void bufferAdd(byte brailleCell[9], byte buffer[4][9], int * rear, int * queEmpty)
{
  int i;
  if (*rear == 3) //end of array reached, 3 is last index
  {
    *rear = 0; //next location circles to front, which is index 0
  } 
  else
  {
    (*rear)++; //not at end of array, increment as normal
  }
  for (i = 0; i < 9; i++) //9 button inputs to copy over
  {
    buffer[*rear][i] = brailleCell[i]; //adds buttons read one at a time to the buffer at index of rear
  }
  *queEmpty = FALSE; //input was added to the bufer so it is no longer empty
}

//Updates buffer based on who many brailleCells were used for the character determination
void updateQue(int * front, int * rear, int * cellsUsed, int * queEmpty)
{
  if (*cellsUsed == 4) //All four cells in buffer was used, buffer is now empty
  {
    //resets buffer to empty values
    *front = 0;
    *rear = 0;
    *queEmpty = TRUE;
    *cellsUsed = 0;
    return;
  }
  if ((*front + *cellsUsed) <= 3) //no need to circle around to front
  {
    *front = *front + *cellsUsed; //moves the front therefore removing the translated entries
  }
  else
  {
    *front = (*front + *cellsUsed) - 4; //if greater than 3 restarts from zero to take into account circle
  }
  *cellsUsed = 0; //all used cells accounted for
}

//resets que back to starting conditions because an enter key was pressed
void enter_updateQue(int * front, int * rear, int * queEmpty)
{
  *front = 0;
  *rear = 0;
  *queEmpty = TRUE;
}

//calculates how many brailleCells are in the que
int lenQue(int front, int rear, int queEmpty)
{
  int ret = 0;
  if (queEmpty == TRUE)
  {
    ret = 0; //empty, no elements
  }
  else if (rear > front) //linear so length is not based on circular effects
  {
    ret = rear - front; //rear will be greater than front and will return the number of characters
  }
  else if (rear == front) //buffer is full
  {
    ret = 4; //buffer is size 4
  }
  else //circular effects need to be taken into account
  {
    ret = (4 - front) + rear; //the end of que has an index smaller than the front, this equation calculates spaces between front and rear 
  }
  return ret;
}

//orders que so indexes 0-3 are in order, for ease of comparison
//simplifies the process of translating because those functions can now assume that buffer is ordered as a que
void queToArray(byte buffer[4][9], int *front, byte arrToCheck[4][9], int buffer_len)
{
  int i;
  int j;
  for (i = 0; i <buffer_len; i++) //iterate through length of buffer
  {
    for (j = 0; j < 9; j++) //iterate through each key input and transfer 1s and 0s
    {
      if (((*front) + 1 + i) <= 3) //circular effects not relevant
      {
        arrToCheck[i][j] = buffer[(*front)+1+i][j]; //copy specific index to new index location
      }
      else //cirular effects need to be taken into account
      {
        arrToCheck[i][j] = buffer[(*front)-(3-i)][j]; //end of buffer reached so back to smaller indexs being copied to new array
      }
    }
  }
}


//-----------COMPARISON CHECKING FUNCTIONS------------------

//Makes sure that multiple braille cell characters are checked first
int checkCharacter(byte arr[4][9], int len, int * cellsUsed)
{
   int ret = 0;

  //No support for 3 and 4 cell characters yet

   /*if (len == 4)
   {
     ret = NemToASCII(4, arr)
     if (ret != -1)
     {
       *cellsUsed = 4; //determines how many spaces to move que
       return ret;
     }
   }
   if (ret <= 0 && len >= 3)
   {
     ret = NemToASCII(3, arr);
     if (ret != -1)
     {
       *cellsUsed = 3;
       return ret;
     }
   }*/
   if (ret <= 0 && len >= 2) //always check 2 cell before one cell, assumption that 2 cells will take precedence over one cell
   {
     ret = NemToASCII_2cell(arr); //comparison function
     if (ret != -1) //good character is returned 
     {
       *cellsUsed = 2; //two cells were used due to good character returned 
       return ret;
     }
   }
   if (ret <= 0 && len >= 1) //check one cell last
   {
     ret = NemToASCII_1cell(arr); //one cell comparison function
     if (ret != -1) //known character returned
     {
       *cellsUsed = 1; //one cell was used to translate
     }
   }

   return ret;
}

//----------------------CHECKS------------------------------------
int NemToASCII_1cell(byte combo_arr[4][9]){ 
/* DOCUMENTATION
    Team: ISBVI - F2014
    Project: LEAP
    Programmer: Tiger Cheng
    Program Description: 
        Compares each valid character cell combination to input combination and return output as ASCII. Else returns no match or error.
    Arguments: 
        1. Number of Braille cells the function is checking 1. Left in for program readability when calling the function
        2. A two dimensional array containing the input combinations (Make sure single cell arrays are also a pointer to pointers.)
 */

  //DEFINITE VALUES
  int no_match = -1;
  int error = -2; //or whatever valid. can't use NULL
  int bpc = 7; //buttons per cell
    
  //ERROR CHECK - prob didn't need 
  if(combo_arr == NULL)                                   {Serial.print("ERROR: Invalid Cell Array - NULL\n"); return error;}
    
  int nChars = 49; //must match first value in nemeth!!!!!!!!!!
  //CHOOSE CHARACTER SET
  char characters[50] = "ablprvq&(fghkmnxyozucde12st)678ijw345'/+90-^:.,# "; //fix * and make dot fix void, void is for delete
  //update index 7 for space
  const byte nemeth[49][7] = {{1,0,0,0,0,0,0}, //a
                           {1,1,0,0,0,0,0}, //b
                           {1,1,1,0,0,0,0}, //l
                           {1,1,1,1,0,0,0}, //p
                           {1,1,1,0,1,0,0}, //r
                           {1,1,1,0,0,1,0}, //v
                           {1,1,1,1,1,0,0}, //q
                           {1,1,1,1,0,1,0}, //and
                           {1,1,1,0,1,1,0}, //(
                           //{1,1,1,1,1,1,0}, //void, not ascii 
                           {1,1,0,1,0,0,0}, //f
                           {1,1,0,1,1,0,0}, //g
                           {1,1,0,0,1,0,0}, //h
                           {1,0,1,0,0,0,0}, //k
                           {1,0,1,1,0,0,0}, //m
                           {1,0,1,1,1,0,0}, //n
                           {1,0,1,1,0,1,0}, //x
                           {1,0,1,1,1,1,0}, //y
                           {1,0,1,0,1,0,0}, //o
                           {1,0,1,0,1,1,0}, //z
                           {1,0,1,0,0,1,0}, //u
                           {1,0,0,1,0,0,0}, //c
                           {1,0,0,1,1,0,0}, //d
                           {1,0,0,0,1,0,0}, //e
                           //{1,0,0,0,0,1,0}, // dot, not ascii
                           {0,1,0,0,0,0,0}, //1
                           {0,1,1,0,0,0,0}, //2
                           {0,1,1,1,0,0,0}, //s
                           {0,1,1,1,1,0,0}, //t
                           {0,1,1,1,1,1,0}, //)
                           {0,1,1,0,1,0,0}, //6
                           {0,1,1,0,1,1,0}, //7
                           {0,1,1,0,0,1,0}, //8
                           {0,1,0,1,0,0,0}, //i
                           {0,1,0,1,1,0,0}, //j
                           {0,1,0,1,1,1,0}, //w
                           {0,1,0,0,1,0,0}, //3
                           {0,1,0,0,1,1,0}, //4
                           {0,1,0,0,0,1,0}, //5
                           {0,0,1,0,0,0,0}, //'
                           {0,0,1,1,0,0,0}, // /
                           {0,0,1,1,0,1,0}, //+
                           {0,0,1,0,1,0,0}, //9
                           {0,0,1,0,1,1,0}, //0
                           {0,0,1,0,0,1,0}, //-       
                           {0,0,0,1,1,0,0}, //^
                           {0,0,0,1,1,1,0}, //:
                           {0,0,0,1,0,1,0}, //.
                           {0,0,0,0,0,1,0}, //,
                           {0,0,1,1,1,1,0}, //#
                           {0,0,0,0,0,0,1}}; // space

  //COMPARE (entire table to result)
  int i; //iterator of valid characters
  int j; //iterators of key combinations
  int match; //match flag
  
  //check space first because space bar goes down each time. want only space button hit
  for (j = 0; j < 7; j++)
  {
    match = (nemeth[48][j] == combo_arr[0][j]) ? TRUE : FALSE;
    if (match == FALSE)
    {
      break;
    }
  }
  if (match == TRUE) //space bar was hit
  {
    return (int)characters[48]; //THIS VALUE MUST MATCH SPACEBAR INDEX
  }
  
  for(i = 0; i < nChars-1; i++){
    for (j = 0; j < 6; j++) { //less than 6 because index 6 is space button which can be ignored
          match = (nemeth[i][j] == combo_arr[0][j]) ? TRUE : FALSE; //ternary operation 
          if (match == FALSE) {break;} //move onto next character
      }
      if(match == TRUE) //valid character found
          {return (int)characters[i];} //return ascii value of that character
  }
  return no_match; //unable to find known character
}

//2 cell comparinson function 
int NemToASCII_2cell(byte combo_arr[4][9])
{
  //update index 7 for space
  int nChars = 38; //MUST match the first value of twoCellNemeth
  const char characters [39] = "-][%*}{/$=><ABLPRVQFGHKMXYOZUCDESTIJWN";  
  const byte twoCellNemeth[38][12] = {{0,0,1,0,0,1,1,0,0,0,0,1},//-
                           {0,0,1,0,0,1,0,1,1,1,1,1}, //]
                           {0,0,1,0,0,1,1,1,1,0,1,1}, //[
                           {0,0,1,0,0,1,0,0,1,0,1,1}, //%
                           {0,0,1,0,0,1,1,0,0,0,0,1}, //*
                           {0,0,0,1,0,1,0,1,1,1,1,1}, //}
                           {0,0,0,1,0,1,1,1,1,0,1,1}, //{
                           {0,0,0,1,0,1,0,0,1,1,0,0}, // /
                           {0,0,0,1,0,1,0,1,1,1,0,0}, //$
                           {0,0,0,1,0,1,1,0,1,0,0,0}, //=
                           {0,0,0,1,0,1,0,1,0,0,0,0}, //>
                           {0,0,0,0,1,0,1,0,1,0,0,0}, //<
                           {0,0,0,0,0,1,1,0,0,0,0,0}, //A
                           {0,0,0,0,0,1,1,1,0,0,0,0}, //B
                           {0,0,0,0,0,1,1,1,1,0,0,0}, //L
                           {0,0,0,0,0,1,1,1,1,1,0,0}, //P
                           {0,0,0,0,0,1,1,1,1,0,1,0}, //R
                           {0,0,0,0,0,1,1,1,1,0,0,1}, //V
                           {0,0,0,0,0,1,1,1,1,1,1,0}, //Q
                           {0,0,0,0,0,1,1,1,0,1,0,0}, //F
                           {0,0,0,0,0,1,1,1,0,1,1,0}, //G
                           {0,0,0,0,0,1,1,1,0,0,1,0}, //H
                           {0,0,0,0,0,1,1,0,1,0,0,0}, //K
                           {0,0,0,0,0,1,1,0,1,1,0,0}, //M
                           {0,0,0,0,0,1,1,0,1,1,0,1}, //X
                           {0,0,0,0,0,1,1,0,1,1,1,1}, //Y
                           {0,0,0,0,0,1,1,0,1,0,1,1}, //O
                           {0,0,0,0,0,1,1,0,1,0,0,1}, //Z
                           {0,0,0,0,0,1,1,0,0,1,0,0}, //U
                           {0,0,0,0,0,1,1,0,0,1,0,0}, //C
                           {0,0,0,0,0,1,1,0,0,1,1,0}, //D
                           {0,0,0,0,0,1,1,0,0,0,1,0}, //E
                           {0,0,0,0,0,1,0,1,1,1,0,0}, //S
                           {0,0,0,0,0,1,0,1,1,1,1,0}, //T
                           {0,0,0,0,0,1,0,1,0,1,0,0}, //I
                           {0,0,0,0,0,1,0,1,0,1,1,0}, //J
                           {0,0,0,0,0,1,0,1,0,1,1,1}, //W
                           {0,0,0,0,0,1,1,0,1,1,1,0}}; //N
                           //{0,1,0,0,1,0,1,0,1,0,1,0}, //sqrt not in characters, not ascii
                           //{0,0,1,0,0,1,1,0,0,1,0,0}} //cent sign not in characters, not ascii
  
  int no_match = -1;
  int error = -2; //or whatever valid. can't use NULL
  int bpc = 6; //buttons per cell

  //COMPARE (entire table to result)
  int i; //iterator of valid characters
  int j; //iterators of key combinations
  int match; //match flag
    
  for(i = 0; i < nChars; i++){
    for (j = 0; j < 12; j++) { //less than 12 because index is 12
          match = (twoCellNemeth[i][j] == combo_arr[j/bpc][j%bpc]) ? TRUE : FALSE; //division and modulus take into account that there are only six elements to check in each index of combo_arr, check two indexes of 6 with one index of 12 for twoCellNemeth
          if (match == FALSE) {break;} //move to next character
      }
      if(match == TRUE) //character found at end checking two cells
          {return (int)characters[i];} //return ascii value of character
  }
  return no_match; //no known character found
}


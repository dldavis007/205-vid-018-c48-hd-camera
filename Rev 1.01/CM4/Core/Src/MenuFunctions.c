#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Subroutines.h"
#include "mcohw.h"
#include "MenuFunctions.h"
#include "eeprom.h"
#include "main.h"
#include "canNodeIds.h"


#define Disp_Wait_Time1  0.001
#define Disp_Wait_Time2  0.005

extern char Variable_flag;
extern char String_Var_ptr;
extern char Multi_Var_ptr;
extern struct MenuStack MenuStackc[];
extern struct MenuStruct const Menuc[];
extern uint8_t StackPointer;
extern char Menu[12][21];
extern char Gen_Flags;
extern uint16_t cam_add;

extern UNSIGNED8 gProcImg[]; 
extern CAN_MSG gTxMsg;
extern uint8_t TC0_RCVD_Data;
extern char F1_Rcvd_Data;

extern struct menu_var SerialNum;
extern struct menu_var NullVar;
extern struct menu_var NullVar2;
extern struct menu_var MicOnOff;
extern struct menu_var CamTag;

extern uint32_t Timer1;
extern uint32_t MenuTimer;
extern uint32_t IncSpeedUpTimer;

extern uint16_t VirtAddVarTab[NB_OF_VAR];
extern uint16_t EEDataTable[NB_OF_VAR];

extern uint8_t fast_inc;

int8_t  CursorDownFlag;
int8_t  CursorUpFlag;
int8_t  SelectFlag;
char EE_CamTag[STR_VALUE_LEN];

char UpdateMenu = 0;
char UpdateArrayVar = 0;
char AcceptKeys = 0;
char StoreFlag = 0;


int NullFunction(void)
{
    return 0;
}

//This gets the variable pointed to by the Menu/Cursor/Multi_Var_ptr
struct menu_var *getVariable(void) 
{
     int k;
	 int j = 1;
	 struct menu_var *var; 
	 
	 k = FindMenu();
	 var = Menuc[k].VarPntr[MenuStackc[StackPointer].CursorPos+MenuStackc[StackPointer].FirstLine - 1];
	 while(j < Multi_Var_ptr)
	 {
	     var = var->next_var;
		 j++;
	 }
     return var;
}

//This displays the "String Type" variable with a '-' in the current "String_Var_ptr" location
void StdVarFunc_String(struct menu_var *var) 
{
	float old_value; 
    getvalue(var, String_Var_ptr - 1);
    old_value = var->value;
    var->value = var->max + 1; 	 //the last char in the enum list is used as a cursor
    getstrval(var);				 //put it in the string temporarily   
    LoadMenu (MenuStackc[StackPointer].Index);
    var->value = old_value;
    getstrval(var);
}

// This puts the next variable in the Menu and updates the pointers "Multi_Var_ptr", "String_Var_ptr", "Variable_flag"
void next_variable(struct menu_var *var)
{
    if (var->next_var) // See if more linked variables
    {
        Multi_Var_ptr++;
        var = getVariable();   // Get the variable pointed to by Multi_Var_ptr
        if (var->len_str < 0)  // See if "String Type"
        {
            String_Var_ptr = 1;
            StdVarFunc_String(var); //Display "String Type"   ">-xxx"
        }
    	else //"Standard" numeric or enum variable here
    	{
            if (!Multi_Var_ptr){
                Variable_flag = 0; //This removes Variable Cursor when the menu is loaded
            }			
        	LoadMenu (MenuStackc[StackPointer].Index);
        	if (!Multi_Var_ptr){
                InsertCursor();
            }
        }
	
    }
	else // No more linked variables, cursor back to beginning of line
	{
	    Variable_flag = 0;
		Multi_Var_ptr = 0;
		String_Var_ptr = 0;
		UpdateArrayVar = 0;
		LoadMenu(MenuStackc[StackPointer].Index);
		InsertCursor();
	}
    
}

// This points to the next char in "String Type" variables and puts in Menu, or goes to next variable
void Next_Char_In_String_Var(struct menu_var *var) 
{
    int i;
    if(++String_Var_ptr > abs(var->len_str)) //End of "String Type" variable here
    {
	    next_variable(var);
    }
    else
    {
	    StdVarFunc_String(var);
    }
}

//When a menu item is first select, this sets the UpdateArrayVar to cause 
//a variable array to be update
int ArrayVarFunction(void)
{
    UpdateArrayVar = 8;
	return StdVarFunction();
}

void UpdateArrayVariables(struct menu_var *var, char num)
{
    char tempstr[12];
    int i = 0;
    
	if (num)
	{
        var = getVariable();
        strncpy (tempstr, &var->str_value[0], sizeof(tempstr));
        while (i++ < num - 1)
        {
            var++;
        	strncpy(&var->str_value[0], tempstr,var->len_str);
        	getvalue(var,0);
        }
		LoadMenu(MenuStackc[StackPointer].Index);
		DisplayTitler();
	}
}

//When a menu item is first select, this skips the first variable on the line (multi-variable)
//The first variable can be used to display info such as temperature
int SkipVarFunction(void)
{
    int i;
	struct menu_var *var;
	
	if (!Variable_flag) //Here the first time select is pushed
	{
	    i = FindMenu(); 
	    var = Menuc[i].VarPntr[MenuStackc[StackPointer].CursorPos+MenuStackc[StackPointer].FirstLine-1];
	    //var is the 1st variable pointed to in the menu
		next_variable(var);
		Variable_flag = 1;
	}
    
	return StdVarFunction();
}

//When a menu item is first select, this sets the Variable
int StdVarFunction (void)
{
    int i;
	struct menu_var *var;
	float old_value; 
	
	i = FindMenu(); 
	var = Menuc[i].VarPntr[MenuStackc[StackPointer].CursorPos + MenuStackc[StackPointer].FirstLine - 1];
    //var is the 1st variable pointed to in the menu
	
	if (!Variable_flag)   //Here the first time select is pushed
    {
		Variable_flag = 1;
		Multi_Var_ptr = 0;
		String_Var_ptr = 0;
		
        if (var->next_var)
		    Multi_Var_ptr = 1;
			
		if (var->len_str < 0)	//puts a cursor char in "String type" variables <<<<<<<<<<<<<<<<<<
		{
    		String_Var_ptr = 1;
			StdVarFunc_String(var);
		}
		else			   //non "String type" variables
		{
            LoadMenu(MenuStackc[StackPointer].Index);
		}
    }
    else //Here if select had already been pushed, see if more variables or string variable, or end if needed
    {
	    var = getVariable();// Get the variable pointed to by Multi_Var_ptr
		 
		//see if "String type" before going to next multi var
		if (var->len_str < 0) //"String type"
		{
		    Next_Char_In_String_Var(var);  
		}
		else
		{
		    next_variable(var);
		}
    }

    DisplayTitler();
    return 0;
}

int SerVarFunction(void)
{
    int i; 
	struct menu_var *var;

    if (EEDataTable[SerNum1VirtIndex] >= '0' && EEDataTable[SerNum1VirtIndex] <= '9')
	{
	    return 0;
	}	

	i = FindMenu(); 
	var = Menuc[i].VarPntr[MenuStackc[StackPointer].CursorPos + MenuStackc[StackPointer].FirstLine - 1];   //var is the 1st variable pointed to in the menu
	    
	if (!Variable_flag) //Here the first time select is pushed
    {
		Variable_flag = 1;
    	String_Var_ptr = 1;
		StdVarFunc_String(var);
	}
	else  //Here if select had already been pushed, see if more variables or string variable, or end if needed
	{
	   var = getVariable();  // Get the variable pointed to
	   Next_Char_In_String_Var(var); 
	}

    DisplayTitler();
 	return 0;
}


int ExitMenu(void)
{
    DeSelect();
    return 0;
}


void CursorUp(void)
{
    if (Variable_flag)
    {
        IncVariable();
		UpdateArrayVariables(getVariable(), UpdateArrayVar); 	
    }
    else
    {
       MenuStackc[StackPointer].CursorPos--;
       LoadMenu (MenuStackc[StackPointer].Index);
       InsertCursor();
       DisplayTitler();
    }
}

void CursorDown(void)
{
    if (Variable_flag)
    {
         DecVariable();
		 UpdateArrayVariables(getVariable(), UpdateArrayVar);
    }
    else
    {
       MenuStackc[StackPointer].CursorPos++;
       LoadMenu(MenuStackc[StackPointer].Index);
       InsertCursor();
       DisplayTitler();
    }
}

//Updates and returns the value member corresponding to the str_value member
//-OR- gets the enum number corresponding to the str_value member
//-OR- gets the enum number of the char pointed to by index for "String type" variables
float getvalue(struct menu_var* var, char index)
{
    char tempstr[120];
    char *token;
    int i;

    if (strlen(var->str_enum) && var->len_str >= 0)  //regular enum list
    {
        strncpy(tempstr, var->str_enum, sizeof(tempstr));

        for (i = 1; i <= var->max; i++)
        {
            if (i == 1)
                token = strtok(tempstr, ",");
            else
                token = strtok(NULL, ",");

			if (!strcmp(var->str_value, token))
			{
			    var->value = i;
				break;
			}
        }
	}
	else if (strlen(var->str_enum) && var->len_str < 0) //"String type" enum list
	{
        strncpy(tempstr, var->str_enum, sizeof(tempstr));

        for (i = 1; i <= var->max; i++)
        {
            if (i == 1)
                token = strtok(tempstr, ",");
            else
                token = strtok(NULL, ",");

			if (*(var->str_value+index) == *token)
			{
			    var->value = i;
				break;
			}
        }
	}
	else if (strlen(var->str_enum) == 0) //"Numeric type" variable, get value of string
    {
        var->value = atof(var->str_value);
    }
	return var->value;
}

//Creates a string from the value member or the enum member
//assigns the string to ->str_value, left pads with space so length = ->len_str
//also returns a pointer to the string
char * getstrval (struct menu_var* var)
{
    char tempstr[100];
    char *token;
    int i;

    if (strlen(var->str_enum) && var->len_str >= 0) //"Enum Type" variable here
    {
        strncpy(tempstr, var->str_enum, sizeof(tempstr));
        token = strtok(tempstr, ",");
        for (i = 0; i < var->value - 1; i++)
        {
            token = strtok(NULL, ",");
        }
        strncpy(var->str_value, token, var->len_str); //The enum string without leading spaces should be shorter than len(var->str_value)
		sprintf(tempstr, "%*s%s", 10, " ", var->str_value); //Use tempstr to pad with leading spaces
		strncpy(var->str_value, tempstr + strlen(tempstr) - abs(var->len_str), var->len_str); //put back in var->str_value
    }
    else if (strlen(var->str_enum) && var->len_str < 0) //"String Type" variables here, get char (using value) from enum and put in string location indicated by String_Var_ptr 
	{
		strncpy(tempstr, var->str_enum, sizeof(tempstr)); 
									   
        for (i = 0; i < var->value; i++)   
        {							   
            if (i == 0)
			   token = strtok(tempstr, ",");
			else
			   token = strtok(NULL, ","); 
        }
        var->str_value[String_Var_ptr - 1] = *token;
	}
	else //"Numeric Type" variable
    {
        tempstr[0] = 0;
		if (var->dec_pos)
		    sprintf(tempstr, "%*s%#.*f", 10, " ", var->dec_pos, var->value); //force decimal if float, use tempstr so it won't overflow var->str_value
        else
		    sprintf(tempstr, "%*s%.*f", 10, " ", var->dec_pos, var->value); //don't force decimal on int, use tempstr so it won't overflow var->str_value
			strncpy(var->str_value, tempstr + strlen(tempstr) - abs(var->len_str), var->len_str); //Put back in var->str_value
    }
	return var->str_value;
}

char FindMenu (void)
{
     int k;
     //Find out which menu we are on
     for (k = 0; k < MenuSize; k++)
     {
        if (Menuc[k].Index[0] == MenuStackc[StackPointer].Index[0] && Menuc[k].Index[1] == MenuStackc[StackPointer].Index[1] && 
             Menuc[k].Index[2] == MenuStackc[StackPointer].Index[2] && Menuc[k].Index[3] == MenuStackc[StackPointer].Index[3])
        {
		    break;
		}
	 }
	 return k;
}

void IncVariable (void)
{
	 incvar(getVariable());
     LoadMenu (MenuStackc[StackPointer].Index);
     DisplayTitler();
}

//Increments a variable using the inc member
//Wraps around to min value if max value is exceeded
//assigns the equivalent string to the str_value member
//also returns a pointer to the string
char *incvar (struct menu_var *var)
{
	getvalue(var, String_Var_ptr - 1);  //String_Var_ptr points to the char within var->str_value that is being modified
					 			        //the enum value is retrieved and put in var->value so "incvar" and "decvar" can use it
    IncSpeedUpTimer = MenuTimer * 1.1;
    
    if (fast_inc && var->max > 50)
        var->value = var->value + var->inc * 10;
	else
	{
        var->value = var->value + var->inc;
		
		if(var->value > var->max)
               var->value = var->min;   
	}
		
    if(var->value > var->max + var->inc / 10)
        var->value = var->min;
	
    return getstrval(var);
}

void DecVariable (void)
{
	 decvar(getVariable());
     LoadMenu (MenuStackc[StackPointer].Index);
     DisplayTitler();
}

//Decrements a variable using the inc member
//Wraps around to max value if min value is exceeded
//assigns the equivalent string to the str_value member
//also returns a pointer to the string
char *decvar (struct menu_var *var)
{
 	getvalue(var, String_Var_ptr - 1);  // String_Var_ptr points to the char with in var>str_value that is being modified
					 			        // the enum value is retrieved and put in var->value so "incvar" and "decvar" can use it
    IncSpeedUpTimer = MenuTimer * 1.1;
    
    if (fast_inc && var->max > 50)
        var->value = var->value - var->inc * 10;
	else
	{
        var->value = var->value - var->inc;
		
		if(var->value < var->min)
		      var->value = var->max;
	}	
		
    if(var->value < var->min - var->inc / 10)
        var->value = var->max;
    
    return getstrval(var);
}


void Select (void)
{
     int k;
     //Save current menu pointers
     MenuStackc[StackPointer+1].Index[0]=MenuStackc[StackPointer].Index[1];
     MenuStackc[StackPointer+1].Index[1]=MenuStackc[StackPointer].Index[2];
     MenuStackc[StackPointer+1].Index[2]=MenuStackc[StackPointer].Index[3];
     MenuStackc[StackPointer+1].Index[3] = MenuStackc[StackPointer].CursorPos + MenuStackc[StackPointer].FirstLine;
     
     //look for selected menu 
     for (k = 0; k < MenuSize; k++)
     {
        if (Menuc[k].Index[0] == MenuStackc[StackPointer+1].Index[0] && Menuc[k].Index[1] == MenuStackc[StackPointer+1].Index[1] && 
            Menuc[k].Index[2] == MenuStackc[StackPointer+1].Index[2] && Menuc[k].Index[3] == MenuStackc[StackPointer+1].Index[3])
        {
             //If found increment stackpointer and initialize cursor
             StackPointer++;
             //Cursor to menu entry 1
             MenuStackc[StackPointer].CursorPos = 1;
             MenuStackc[StackPointer].FirstLine = 0;
             LoadMenu (MenuStackc[StackPointer].Index);
             InsertCursor();
             DisplayTitler();
             break;
        }
     } 

     if (k >= MenuSize)
     {
         //Find out which menu we are on
         for (k = 0; k < MenuSize; k++)
         {
            if (Menuc[k].Index[0] == MenuStackc[StackPointer].Index[0] && Menuc[k].Index[1] == MenuStackc[StackPointer].Index[1] && 
                Menuc[k].Index[2] == MenuStackc[StackPointer].Index[2] && Menuc[k].Index[3] == MenuStackc[StackPointer].Index[3])
            {
                 //Execute function for that entry 
                 Menuc[k].FunctPtr[(MenuStackc[StackPointer].CursorPos + MenuStackc[StackPointer].FirstLine) - 1]();
                 break;
            }
        }
   }
}

void DeSelect (void)
{
     int i;
	 short hexNodeID;
	 
     if (StackPointer)
     {
        StackPointer--;
        LoadMenu(MenuStackc[StackPointer].Index);
        InsertCursor();
        DisplayTitler();
     }
     else
     {
        MenuStackc[StackPointer].Index[0] = 0;
        MenuStackc[StackPointer].Index[1] = 0;
        MenuStackc[StackPointer].Index[2] = 0;
        MenuStackc[StackPointer].Index[3] = 0;
        Gen_Flags &= ~Gen_Flags_Menu_Active;
        ClearTitler();
        Timer1 = RTI_One_Sec * 0.05;
	    while ( Timer1 );
		Send_Menu_Status(0x00);
		for (i = 1; i <= NR_OF_TPDOS; i++)
		{
		    COP_Trig();
			MCO_ProcessStack();
		}
		StoreFlag = 1;
     }
	 Variable_flag = 0;
	 String_Var_ptr = 0;
	 Multi_Var_ptr = 0;
	 UpdateArrayVar = 0;
}



void LoadMenu (char Index[])
{
    int i,j,k,l;

    //find menu
	k = FindMenu();
	
    //Get Menu Title
    for (j = 0; j <= 20; j++)
	{
        Menu[0][j] = Menuc[k].Entry[0][j];
	}
       
    //See if this entry is a valid menu entry, if not start cursor back at top
    if (MenuStackc[StackPointer].CursorPos > 0 && Menuc[k].Entry[MenuStackc[StackPointer].CursorPos + MenuStackc[StackPointer].FirstLine][1] == ' ')
    {
         MenuStackc[StackPointer].CursorPos = 1;
         MenuStackc[StackPointer].FirstLine = 0;
    }
        
    //Allow a maximum of 12 entries, if past 12, start cursor back at top
    if (MenuStackc[StackPointer].CursorPos + MenuStackc[StackPointer].FirstLine >= 12)
    {
         MenuStackc[StackPointer].CursorPos = 1;
         MenuStackc[StackPointer].FirstLine = 0;
    }
        
    //If cursor is too far down, check to see if there is another entry, if so, show it and set cursor to bottom entry.
    if (MenuStackc[StackPointer].CursorPos == 9)
    {
         if (Menuc[k].Entry[9 + MenuStackc[StackPointer].FirstLine][1] != ' ')
         {
            MenuStackc[StackPointer].FirstLine++;
            MenuStackc[StackPointer].CursorPos = 8;
         }
         else
         {
            MenuStackc[StackPointer].CursorPos = 1;
            MenuStackc[StackPointer].FirstLine = 0;
         }
    }

    //If cursor is too far up, and the first entry is not the first line then move firstline up and set cursor to top entry
    if (MenuStackc[StackPointer].CursorPos == 0)
    {
         if (MenuStackc[StackPointer].FirstLine)
         {
            MenuStackc[StackPointer].FirstLine--;
            MenuStackc[StackPointer].CursorPos = 1;
         }
         else
         {
             //Find last entry
             for (i = 11; i > 0; i--)
             {
                 if (Menuc[k].Entry[i][1] != ' ' && Menuc[k].Entry[i][0] == ' ')
                      break;
             }
             //If more than 8 entries, cursor will be at the bottom (8) and first line will be adjusted
             if (i > 8)
             {
                 MenuStackc[StackPointer].CursorPos = 8;
                 MenuStackc[StackPointer].FirstLine = i - 8;
             }
             //Otherwise the cursor will be on that entry with the firstline at 0
             else
             {
                 MenuStackc[StackPointer].CursorPos = i;
                 MenuStackc[StackPointer].FirstLine = 0;
             }
         }
    }

    //Load appropriate menu entries into Menu[i][j]
    for (i = 1; i <= 8; i++)
	{
	    for (j = 0; j <= 20; j++)
		{
            Menu[i][j] = Menuc[k].Entry[i+MenuStackc[StackPointer].FirstLine][j];
		}
	}
	
	//Load any variables
    for (i = 1; i <= 8; i++)
	{
        if (Menuc[k].Pos[i + MenuStackc[StackPointer].FirstLine - 1])
        {
			 struct menu_var *var = Menuc[k].VarPntr[i + MenuStackc[StackPointer].FirstLine - 1];
			 char next_flag = 1;
			 int var_cntr = 1;
             j = 0;    

			 do 
			 {	  
                 int l;
				 for (l = 0; j <= 20; j++, l++)  //up to 11 char variable "STR_VALUE_LEN"
                 { 
                    if (!Multi_Var_ptr && Variable_flag && i == MenuStackc[StackPointer].CursorPos && l == 0) 
                        Menu[i][Menuc[k].Pos[i + MenuStackc[StackPointer].FirstLine - 1] - 1] = '>';

                    if (Multi_Var_ptr == var_cntr && i == MenuStackc[StackPointer].CursorPos && l == 0)
					{
						 Menu[i][Menuc[k].Pos[i + MenuStackc[StackPointer].FirstLine - 1] + j - 1] = '>';
					}
                    
    				if (var->len_str >= 0)
    				{    
    				    getstrval(var);
    			    }
					
    				if (*(var->str_value + l)) //if current char of string isn't a NULL
                    {
                        Menu[i][Menuc[k].Pos[i + MenuStackc[StackPointer].FirstLine - 1] + j] = *(var->str_value + l);
                    }
                    else
                    {
                        break;
                    }
                 }
				 if (var->next_var)
				 {
					 var = var->next_var;
					 Menu[i][Menuc[k].Pos[i + MenuStackc[StackPointer].FirstLine - 1] + j] = ' ';
					 j++;
				  	 var_cntr++;
				 }
				 else
				 {
				     next_flag = 0;
				 }
			 }while(next_flag);
        }            
	}
}

void InsertCursor (void)
{
	//Display Cursor Pointer
    Menu[MenuStackc[StackPointer].CursorPos][0] = '>';
}     

void DisplayTitler (void)
{
	int i,j,k;
	
    PositionDisplay();
	
	gTxMsg.ID = WTX_ID;
	gTxMsg.LEN = 8;
	gTxMsg.BUF[0] = 0;
	gTxMsg.BUF[1] = 0x02;
	gTxMsg.BUF[2] = 'M';
	gTxMsg.BUF[3] = 'e';
	gTxMsg.BUF[4] = 'n';
	gTxMsg.BUF[5] = 'u';
	gTxMsg.BUF[6] = ':';
	gTxMsg.BUF[7] = 0;
	
   	if (!MCOHW_PushMessage(&gTxMsg))
   	{
        // failed to transmit
       	MCOUSER_FatalError(0x8801);
    }
	Timer1 = RTI_One_Sec * Disp_Wait_Time1;
	while(Timer1);
	
	for (i = 0; i <= 8; i++)        //9 lines
	{
	    gTxMsg.ID = WTX_ID;
		gTxMsg.LEN = 7;
		gTxMsg.BUF[0] = 0;
		gTxMsg.BUF[6] = 0;
	    for (j = 0; j <= 15; j += 5)
		{
    	    COP_Trig();
		 	for (k = 0; k <= 4; k++)
			{
		     	gTxMsg.BUF[k + 1] = Menu[i][j + k];
			}
   	    	if (!MCOHW_PushMessage(&gTxMsg))
   			{
                // failed to transmit
       			MCOUSER_FatalError(0x8801);
    		}
			Timer1 = RTI_One_Sec * Disp_Wait_Time2;
			while(Timer1);
			MCO_ProcessStack();
		}
		MCO_ProcessStack();
	}


	gTxMsg.ID = WTX_ID;
	gTxMsg.LEN = 3;
	gTxMsg.BUF[0] = 0;
	gTxMsg.BUF[1] = 0x03;
	gTxMsg.BUF[2] = 0;

   	if (!MCOHW_PushMessage(&gTxMsg))
   	{
        // failed to transmit
       	MCOUSER_FatalError(0x8801);
    }
	Timer1 = RTI_One_Sec * Disp_Wait_Time1;
	while(Timer1);

}

void Display(char buff[27])
{
    static int i,j;
    int len = strlen(buff);
    
    gTxMsg.ID = WTX_ID;
    gTxMsg.LEN = 8;
    gTxMsg.BUF[0] = NODE_ID; //not sent
    gTxMsg.BUF[1] = 0x02; //starting w/ Rev 2.0, <STX> indicates beginning of command
    
    for (i = 0; i <= 5; i++)
    {
        gTxMsg.BUF[i + 2] = buff[i];
    }
    
    if (!MCOHW_PushMessage(&gTxMsg))
        MCOUSER_FatalError(0x8801);
    
    Timer1 = RTI_One_Sec * Disp_Wait_Time1;
    while(Timer1);
    
    gTxMsg.BUF[0] = NODE_ID; 
    
    for (i = 6; i < len; i += 7)
    {
        if (i >= 27)
            break;
        
        for (j = 0; j <= 6; j++)
        {
            gTxMsg.BUF[j+1] = buff[i+j]; //6-12; 13-19; 14-20
        }
        
        if (!MCOHW_PushMessage(&gTxMsg))
            MCOUSER_FatalError(0x8801);
        
        Timer1 = RTI_One_Sec * Disp_Wait_Time2;
        while(Timer1);
        
        MCO_ProcessStack();
    }
    
    gTxMsg.ID = WTX_ID;
    gTxMsg.LEN = 3;
    gTxMsg.BUF[0] = NODE_ID;
    gTxMsg.BUF[1] = 0x03;
    gTxMsg.BUF[2] = 0;
    
    if (!MCOHW_PushMessage(&gTxMsg))
        MCOUSER_FatalError(0x8801);
    
    Timer1 = RTI_One_Sec * Disp_Wait_Time1;
    while(Timer1);
    
}

void PositionDisplay (void)
{
	gTxMsg.ID = WTX_ID;
	gTxMsg.LEN = 8;
	gTxMsg.BUF[0] = 0;
	gTxMsg.BUF[1] = 0x02;
	gTxMsg.BUF[2] = 'H';
	gTxMsg.BUF[3] = 'o';
	gTxMsg.BUF[4] = 'm';
	gTxMsg.BUF[5] = 'e';
	gTxMsg.BUF[6] = ':';
	gTxMsg.BUF[7] = 0x03;
	
   	if (!MCOHW_PushMessage(&gTxMsg))
   	{
        // failed to transmit
       	MCOUSER_FatalError(0x8801);
    }
	Timer1 = RTI_One_Sec * Disp_Wait_Time1;
	while(Timer1);
}

void ClearTitler (void)
{
    int i;
	CAN_MSG gTxMsg;
	CAN_MSG *pTxMsg = &gTxMsg;

	gTxMsg.ID = WTX_ID;
	gTxMsg.LEN = 8;
	gTxMsg.BUF[0] = 0;
	gTxMsg.BUF[1] = 0x02;
	gTxMsg.BUF[2] = 'C';
	gTxMsg.BUF[3] = 'l';
	gTxMsg.BUF[4] = 'r';
	gTxMsg.BUF[5] = ':';
	gTxMsg.BUF[6] = 0x03;
	gTxMsg.BUF[7] = 0;       //place-holder character, RF will not transmit null

	
	if (!MCOHW_PushMessage(&gTxMsg))
   	{
        // failed to transmit
       	MCOUSER_FatalError(0x8801);
    }
	Timer1 = RTI_One_Sec * Disp_Wait_Time1;
	while(Timer1);

}


int menu_function (void)
{
    int i;
	
    if (StoreFlag)  
    {
        StoreFlag = 0;
        //if detects CamTag is different, send msg to controller to update camera list
        if (strncmp(&CamTag.str_value[0], &EE_CamTag[0], STR_VALUE_LEN - 1) != 0)
        {
            Update_Cam_List();        
        }

        Save_Serial_Num();
        Save_Variables();     
    }
		
    if (TC0_RCVD_Data != ( gProcImg[OUT_digi_2]<<8 | gProcImg[OUT_digi_1] ))
    {
        TC0_RCVD_Data = gProcImg[OUT_digi_2]<<8 | gProcImg[OUT_digi_1];
    }
	
	if (F1_Rcvd_Data != gProcImg[OUT_digi_3])
	{
	    F1_Rcvd_Data = gProcImg[OUT_digi_3];
	}

    if ((gProcImg[OUT_digi_6] & 0x08) &&              //command to activate menu
            (gProcImg[OUT_digi_7] == (cam_add & 0x00FF)) &&         //lsb - old address
                (gProcImg[OUT_digi_8]<< 8 ==  (cam_add & 0xFF00)) &&
                 !(Gen_Flags & Gen_Flags_Menu_Active))
    {
	    gProcImg[OUT_digi_6] = 0x00;
        gProcImg[OUT_digi_7] = 0x00;
        gProcImg[OUT_digi_8] = 0x00;
        gProcImg[OUT_digi_4] = (cam_add & 0x00FF);
        gProcImg[OUT_digi_5] = (cam_add & 0xFF00) >> 8;
        camera_ON(true);      //turn camera on
		if (MicOnOff.value == 2)
		    mic_setEnabled(true);

        //turn off other cameras
        gTxMsg.ID = 0x421;
        gTxMsg.LEN = 2; 
        gTxMsg.BUF[0] = cam_add;
        gTxMsg.BUF[1] = cam_add >> 8;  
		if (!MCOHW_PushMessage(&gTxMsg))
        {
            // failed to transmit
            MCOUSER_FatalError(0x8801);
        } 
			
        MenuTimer = MenuTime * 2;
      
        gProcImg[OUT_digi_0] &= ~0x01;
		
        Timer1 = RTI_One_Sec * .10;
        while(Timer1);
        gProcImg[IN_digi_0] |= 0x01;
        MCO_ProcessStack();
        Timer1 = RTI_One_Sec * .10;
        while(Timer1);
               
        StackPointer = 0;
        MenuStackc[StackPointer].Index[0] = 0;
        MenuStackc[StackPointer].Index[1] = 0;
        MenuStackc[StackPointer].Index[2] = 0;
        MenuStackc[StackPointer].Index[3] = 0;
        MenuStackc[StackPointer].CursorPos = 1;
        MenuStackc[StackPointer].FirstLine = 0;
        Gen_Flags |= Gen_Flags_Menu_Active;
        
        LoadMenu(MenuStackc[StackPointer].Index);
        InsertCursor();
		UpdateMenu = 1;
		AcceptKeys = 0;
        CursorDownFlag = 0;
        CursorUpFlag = 0;
        SelectFlag = -1;
		Send_Menu_Status(0x01);
    }

	if (AcceptKeys)
	{
    	if ((TC0_RCVD_Data & TeleData_Down) && CursorDownFlag == 0)
    		CursorDownFlag = 1;
    	else if (CursorDownFlag == -1 && !( TC0_RCVD_Data & TeleData_Down ))
    		CursorDownFlag = 0;
    
    	if ((TC0_RCVD_Data &  TeleData_Up) && CursorUpFlag == 0)
    		CursorUpFlag = 1;
    	else if (CursorUpFlag == -1 && !(TC0_RCVD_Data & TeleData_Up))
    		CursorUpFlag = 0;
    
    	if ((TC0_RCVD_Data & TeleData_Select) && SelectFlag == 0)
    		SelectFlag = 1;
    	else if (SelectFlag == -1 && !(TC0_RCVD_Data & TeleData_Select))
    		SelectFlag = 0;
	}
	
	if (!MenuTimer)
	{
		MenuTimer = MenuTime;
		AcceptKeys = 1;
		
		if (Gen_Flags & Gen_Flags_Menu_Active)
		{
			if (CursorDownFlag)
			{
				CursorDownFlag = -1;
				CursorDown();				
			}
			if (CursorUpFlag)
			{
				CursorUpFlag = -1;
				CursorUp();			
			}			 
			if (SelectFlag == 1)
			{
				MenuTimer = MenuTime * 2;                          //briefly disable select after menu is selected                 
				Select();	
				SelectFlag = -1;		
			}
			if (UpdateMenu)
			{
				LoadMenu(MenuStackc[StackPointer].Index);
				if (!Variable_flag)
					InsertCursor();
				DisplayTitler();
				UpdateMenu = 0;
			}
		}
	
        if (UpdateMenu)
        {
            if (Gen_Flags & Gen_Flags_Menu_Active)
            {
                LoadMenu(MenuStackc[StackPointer].Index);
                if (!Variable_flag)
                    InsertCursor();
                DisplayTitler();
            }
            else
            {
                ClearTitler();
            }
            UpdateMenu = 0;
        }
	}

    if (Gen_Flags & Gen_Flags_Menu_Active)
	    return 1;
	else
	    return 0;
} 

void Save_Variables(void)
{
    int offset = 0;
    char tempstr[NB_OF_VAR] = {0};
    uint16_t virt_address_index = 20; //Starting virtual address index for menu variables

    for (struct menu_var *var = &NullVar; var <= &NullVar2; ++var)
    {
        size_t len = 0;
        while (len < STR_VALUE_LEN && var->str_value[len]) ++len;
        if (!len || len == STR_VALUE_LEN || offset + len + 1 >= sizeof tempstr)
        {
            printf("Invalid or oversized menu settings; EEPROM unchanged\r\n");
            return;
        }
        memcpy(tempstr + offset, var->str_value, len);
        offset += len;
        tempstr[offset++] = ',';
    }
    tempstr[offset - 1] = 0;
    /* offset includes the terminator, even when the string length is even. */

    //Write to EEPROM
    HAL_FLASH_Unlock();
    for (int i = 0; i < offset; i += 2)
    {
        uint16_t packed = (uint8_t)tempstr[i];

        if (i + 1 < offset)
            packed |= ((uint16_t)(uint8_t)tempstr[i + 1]) << 8;

        uint16_t address = VirtAddVarTab[virt_address_index];
        uint16_t current_value;

        if (EE_ReadVariable(address, &current_value) == 0 && current_value == packed)
        {
            virt_address_index++;
            continue;
        }

        if (EE_WriteVariable(address, packed) == 0)
        {
            EEDataTable[virt_address_index] = packed;
        }
        else
        {
            printf("EEPROM write failed at index %d (value 0x%04X)\r\n", virt_address_index, packed);
            HAL_FLASH_Lock();
            return;
        }

        virt_address_index++;
    }
    HAL_FLASH_Lock();
    strncpy(&EE_CamTag[0], &CamTag.str_value[0], STR_VALUE_LEN - 1); 
}

void Load_Variables(void)
{
    char menu_vars[NB_OF_VAR] = {0}; 
    struct menu_var *var;
    size_t pos = 0;

    uint16_t address = VirtAddVarTab[20];
    uint16_t current_value;

    if (EE_ReadVariable(address, &current_value) == 0 && (current_value & 0x00FF) == 0xFF)  //restore defaults
    {
        Save_Variables();
        Load_Variables();
        Update_Cam_List();
		return;
    }

    for (uint16_t virt_address_index = 20; virt_address_index <= NB_OF_VAR - 1; virt_address_index++)
    {
        uint16_t val;
        if (EE_ReadVariable(VirtAddVarTab[virt_address_index], &val) != 0) 
            break;  //no variables left to read
      
        EEDataTable[virt_address_index] = val;

        // Reserve space for a terminating null even with malformed EEPROM.
        if (pos + 2 > sizeof menu_vars) return;
        menu_vars[pos++] = (char)(val & 0xFF);
        if ((val & 0xFF) == 0) break;
        menu_vars[pos++] = (char)(val >> 8);
        if ((val >> 8) == 0) break;
    }
    if (pos == sizeof menu_vars && menu_vars[pos - 1] != 0) return;

    //Tokenize and assign back to menu structs
    char *token = strtok(menu_vars, ",");
    char *var_ptr = (char *)&NullVar.str_value;

    while (token && var_ptr <= (char *)&NullVar2.str_value)
    {
        strncpy(var_ptr, token, STR_VALUE_LEN - 1);  
        ((char *)var_ptr)[STR_VALUE_LEN - 1] = '\0'; //Ensure null termination

        var_ptr += sizeof(struct menu_var);
        token = strtok(NULL, ",");
    }

    //updates the values of the menu_vars 
    for(var = &NullVar; var <= &NullVar2; var += 1)	
	{
	    getvalue(var, 0);
	}

    strncpy(&EE_CamTag[0], &CamTag.str_value[0], STR_VALUE_LEN - 1); 
}


void Load_Serial_Num ( void )
{
    int i;
    for (i = 0; i < 7; i++)
    {
        EE_ReadVariable(VirtAddVarTab[SerNum1VirtIndex + i],  &EEDataTable[SerNum1VirtIndex + i]);  
    }    
    
    if (EEDataTable[SerNum1VirtIndex] >= '0' && EEDataTable[SerNum1VirtIndex] <= '9') 
    {
        SerialNum.str_value[0] = EEDataTable[SerNum1VirtIndex];
        SerialNum.str_value[1] = EEDataTable[SerNum1VirtIndex + 1];  
        SerialNum.str_value[2] = EEDataTable[SerNum1VirtIndex + 2];  
        SerialNum.str_value[3] = EEDataTable[SerNum1VirtIndex + 3];     
        SerialNum.str_value[4] = EEDataTable[SerNum1VirtIndex + 4];   
        SerialNum.str_value[5] = EEDataTable[SerNum1VirtIndex + 5];  
		SerialNum.str_value[6] = EEDataTable[SerNum1VirtIndex + 6];  
        SerialNum.str_value[7] = 0;
    }
}

void Save_Serial_Num ( void )
{
    int i;

    if (EEDataTable[SerNum1VirtIndex] >= '0' && EEDataTable[SerNum1VirtIndex] <= '9' && SerialNum.str_value[0] >= '0' && SerialNum.str_value[0] <= '9')
	    return;

    HAL_FLASH_Unlock();
    for (i = 0; i < 8; i++)
    {
        if (EE_WriteVariable(VirtAddVarTab[SerNum1VirtIndex + i], (uint16_t)SerialNum.str_value[i]) == 0)
            EEDataTable[SerNum1VirtIndex + i] = (uint16_t)SerialNum.str_value[i];
    }   
    HAL_FLASH_Lock();
}

int RestoreDefaults(void)
{
    ClearTitler();
    HAL_FLASH_Unlock();
    EE_WriteVariable(VirtAddVarTab[20], (EEDataTable[20] & 0xFF00) | 0x00FF);  //NullVar.str_value[0] = 0xFF
    HAL_FLASH_Lock();
    TriggerMode.value = 5;
    getstrval(&TriggerMode);
    Save_Trigger_Mode();
    ResetProc();
    return 0;
}

void UpdateVarByValue (struct menu_var *var, float val)
{
    var->value = val;
	getstrval(var);
}

void UpdateVarByString (struct menu_var *var, char str_val[20])
{
    strcpy(var->str_value, str_val);
    getvalue(var, 0);
}

void Send_Menu_Status (char stat)
{
    gTxMsg.ID = 0x200;
    gTxMsg.LEN = 1;
    gTxMsg.BUF[0] = stat;

    if (!MCOHW_PushMessage(&gTxMsg))
    {
        // failed to transmit
        MCOUSER_FatalError(0x8801);
    }
}

void Update_Cam_List(void)
{
    gTxMsg.ID = 0x2A1;
    gTxMsg.LEN = 3;
    gTxMsg.BUF[0] = 0;
    gTxMsg.BUF[1] = 0;
    gTxMsg.BUF[2] = 0xFF;
    if (!MCOHW_PushMessage(&gTxMsg))
    {
        // failed to transmit
        MCOUSER_FatalError(0x8801);
    }
}

//this function assumes you will always start with line 0 (top line) 
//accepts max 21 characters
//set end flag once done with multiline
//usage example:
//DisplayMultiline(0, "First line", 0);
//DisplayMultiline(1, "second line", 0);
//DisplayMultiline(2, "third line", 0);
//DisplayMultiline(5, "last line", 1);
void DisplayMultiline (int line, char string[21], bool endflag) 
{
 	 unsigned int length;
	 char tempstr[21];
	 int i;
	 
	 strcpy(tempstr, string);
	 
 	 if (line == 0)
	 	clearMenu();
		
	 length = strlen(string);	

	 if (length < 20){
	    for(i = length; i < 20; i++){
			strcat(tempstr, " ");
		}  
	 }
	 
	 strcpy(&Menu[line][0], tempstr);
	 
	 if (endflag)
	 	DisplayTitler();
}

void clearMenu (void)
{
	 int i;
	 for (i=0; i<12; i++)
	 {
	 	 sprintf (&Menu[i][0],"                    ");  //20 spaces
	 }
}

int update_paired_triggerable_unit_menu_wrapper(void)
{
    StdVarFunction();
    if (!Variable_flag) {
        Save_Trigger_Mode();
        update_paired_triggerable_unit();
    }
    return 1;
}

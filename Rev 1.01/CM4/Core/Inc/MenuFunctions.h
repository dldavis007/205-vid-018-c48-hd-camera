#ifndef MenuFunctions_H
#define MenuFunctions_H

#define MenuSize 6
#define MenuStackSize 5
#define STR_VALUE_LEN 12

#define MenuTime RTI_One_Sec * 0.25

struct menu_var
{
	float value;				   // actual value of the variable, or pointer to enum list if an enum type variable
	float inc;					   // increment/decrement value, may be int or float, must be 1 for enum type variable
	float min;					   // minimum value of variable, may be int or float, must point to first enum in list for enum type variable
	float max;					   // maximum value of variable, may be int or float, must point to last enum in list for enum type variable
	char dec_pos;				   // decimal position, zero if variable is an int or an enum
	signed char len_str;		   // length of str_value, pads left with spaces
	char str_value[STR_VALUE_LEN]; // string equivalent of value, or current enum pointed to by value
	const char *str_enum;		   // pointer to comma separated enum list, must be NULL for non-enum variable types
	struct menu_var *next_var;	   // pointer to then next variable if more than one per line
};

int NullFunction(void);
struct menu_var *getVariable(void);
void StdVarFunc_String(struct menu_var *var);
void next_variable(struct menu_var *var);
void Next_Char_In_String_Var(struct menu_var *var);
int ArrayVarFunction(void);
void UpdateArrayVariables(struct menu_var *var, char num);
int SkipVarFunction(void);
int StdVarFunction(void);
int SerVarFunction(void);
int ExitMenu(void);
void CursorUp(void);
void CursorDown(void);
float getvalue(struct menu_var *var, char index);
char *getstrval(struct menu_var *var);
char FindMenu(void);
void IncVariable(void);
char *incvar(struct menu_var *var);
void DecVariable(void);
char *decvar(struct menu_var *var);
void Select(void);
void DeSelect(void);
void LoadMenu(char Index[]);
void InsertCursor(void);
void DisplayTitler(void);
void Display(char buff[27]);
void PositionDisplay(void);
void ClearTitler(void);
int menu_function(void);
void Send_Menu_Status(char stat);

void Load_Variables(void);
void Load_Serial_Num(void);
void Save_Serial_Num(void);
void Save_Variables(void);
void UpdateVarByValue(struct menu_var *var, float val);
void UpdateVarByString(struct menu_var *var, char str_val[20]);
int RestoreDefaults(void);
void DisplayMultiline(int line, char string[21], bool endflag);
void clearMenu(void);
void Update_Cam_List(void);

typedef struct MenuStruct
{
	char Index[4];
	char Entry[12][21];
	char Pos[11];
	struct menu_var *VarPntr[11];
	int (*FunctPtr[11])(void);
} MenuStruct;

typedef struct MenuStack
{
	char Index[4];
	int CursorPos;
	char FirstLine;
} MenuStack;

#endif

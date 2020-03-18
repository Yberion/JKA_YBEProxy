// ==================================================
// DetourPatcher by Deathspike
// --------------------------------------------------
// Collection of functions written to assist in detouring
// functions and patching bytes. It mainly allocates
// trampolines to execute the original bytes and uses
// jumps to go to the new function. Supports both
// Windows and Linux operating systems.
// ==================================================

unsigned char	*Attach( unsigned char *pAddress, unsigned char *pNewAddress );
void			 DisAssemble( unsigned char *iptr0, unsigned int *osizeptr );
unsigned char	*Detach( unsigned char *pAddress, unsigned char *pTramp );
unsigned int	 GetLen( unsigned char *pAddress );
unsigned char	*GetTramp( unsigned char *pAddress, unsigned int iLen );
unsigned int	 InlineFetch( unsigned char *pAddress );
unsigned int	 InlinePatch( unsigned char *pAddress, unsigned char *pNewAddress );
void			 Patch( unsigned char *pAddress, unsigned char bByte );
void			 ReProtect( void *pAddress, int iLen );
void			 UnProtect( void *pAddress, int iLen );

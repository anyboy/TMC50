/* asm.h: various macros to help assembly language writers*/
 
/* Concatenate two names. */
#ifdef __STDC__
# define _ASMCONCAT(A, B) A ## B
#else
# define _ASMCONCAT(A, B) A/**/B
#endif

/* Name of boot code code section. */
#ifndef _BOOT_SECTION
#if __GNUC__ >= 3	
#define _BOOT_SECTION .section .boot, "ax", @progbits
#else
#define _BOOT_SECTION .text
#endif
#endif

/* Default code section. */
#ifndef _TEXT_SECTION
#ifdef _BOOTCODE
#define _TEXT_SECTION _BOOT_SECTION
#else
#define _TEXT_SECTION .text
#endif
#endif

/*
 * Leaf functions declarations.
 */

/* Global leaf function. */
#define LEAF(name) 			\
  	_TEXT_SECTION; 			\
  	.globl	name; 			\
  	.ent	name; 			\
name:

/* Static/Local leaf function. */
#define SLEAF(name) 			\
  	_TEXT_SECTION; 			\
  	.ent	name; 			\
name:

/* Weak leaf function. */
#define WLEAF(name) 			\
  	_TEXT_SECTION; 			\
  	.weakext name; 			\
  	.ent	name; 			\
name:

/* Weak alias leaf function. */
#define ALEAF(name,alias) 		\
  	_TEXT_SECTION; 			\
  	.weakext alias,name; 		\
  	.ent	name; 			\
name:

/*
 * Alternative function entrypoints.
 */

/* Global alternative entrypoint. */
#define AENT(name) 			\
  	.globl	name; 			\
  	.aent	name; 			\
name:
#define XLEAF(name)	AENT(name)

/* Local/static alternative entrypoint. */
#define SAENT(name) 			\
  	.aent	name; 			\
name:
#define SXLEAF(name)	SAENT(name)


/*
 * Leaf functions declarations.
 */

/* Global nested function. */
#define NESTED(name, framesz, rareg)	\
  	_TEXT_SECTION; 			\
  	.globl	name; 			\
  	.ent	name; 			\
	.frame	$sp, framesz, rareg;	\
name:

/* Static/Local nested function. */
#define SNESTED(name, framesz, rareg)	\
  	_TEXT_SECTION; 			\
  	.ent	name; 			\
	.frame	$sp, framesz, rareg;	\
name:

/* Weak nested function. */
#define WNESTED(name, framesz, rareg)	\
  	_TEXT_SECTION; 			\
  	.weakext name; 			\
  	.ent	name; 			\
	.frame	$sp, framesz, rareg;	\
name:

/* Weak alias nested function. */
#define ANESTED(name, alias, framesz, rareg) \
  	_TEXT_SECTION; 			\
    	.weakext alias, name;		\
  	.ent	name; 			\
	.frame	$sp, framesz, rareg;	\
name:

/*
 * Function termination
 */
#define END(name) 			\
  	.size name,.-name; 		\
  	.end	name

#define SEND(name)	END(name)
#define WEND(name)	END(name)
#define AEND(name,alias) END(name)

/*
 * Global data declaration.
 */
#define EXPORT(name) \
  	.globl name; \
  	.type name,@object; \
name:

/*
 * Global data declaration with size.
 */
#define EXPORTS(name,sz) 		\
  	.globl name; 			\
  	.type name,@object; 		\
  	.size name,sz; 			\
name:

/*
 * Weak data declaration with size.
 */
#define WEXPORT(name,sz) 		\
  	.weakext name; 			\
  	.type name,@object; 		\
  	.size name,sz; 			\
name:

/*
 * Global data reference with size.
 */
#define	IMPORT(name, size) 		\
	.extern	name,size

/*
 * Global zeroed data.
 */
#define BSS(name,size) 			\
  	.type name,@object; 		\
	.comm	name,size

/*
 * Local zeroed data.
 */
#define LBSS(name,size) 		\
  	.lcomm	name,size

/*
 * Insert call to _mcount if profiling.
 */
#ifdef __PROFILING__
#define _MCOUNT 			\
	.set push; 			\
	.set noat; 			\
	move	$1,$31; 		\
	jal	_mcount; 		\
  	.set pop
#else
#define _MCOUNT
#endif

/*
 * Macros to handle different pointer/register sizes for 32/64-bit code
 */

/*
 * Size of a register
 */
#define SZREG	4


/*
 * Use the following macros in assemblercode to load/store registers,
 * pointers etc.
 */

#define REG_S		sw
#define REG_L		lw
#define REG_SUBU	subu
#define REG_ADDU	addu


/*
 * How to add/sub/load/store/shift C int variables.
 */

#define INT_ADD		add
#define INT_ADDU	addu
#define INT_ADDI	addi
#define INT_ADDIU	addiu
#define INT_SUB		sub
#define INT_SUBU	subu
#define INT_L		lw
#define INT_S		sw
#define INT_SLL		sll
#define INT_SLLV	sllv
#define INT_SRL		srl
#define INT_SRLV	srlv
#define INT_SRA		sra
#define INT_SRAV	srav



/*
 * How to add/sub/load/store/shift C long variables.
 */

#define LONG_ADD	add
#define LONG_ADDU	addu
#define LONG_ADDI	addi
#define LONG_ADDIU	addiu
#define LONG_SUB	sub
#define LONG_SUBU	subu
#define LONG_L		lw
#define LONG_S		sw
#define LONG_SLL	sll
#define LONG_SLLV	sllv
#define LONG_SRL	srl
#define LONG_SRLV	srlv
#define LONG_SRA	sra
#define LONG_SRAV	srav

//#define LONG		.word
#define LONGSIZE	4
#define LONGMASK	3
#define LONGLOG		2



/*
 * How to add/sub/load/store/shift pointers.
 */

#define PTR_ADD		add
#define PTR_ADDU	addu
#define PTR_ADDI	addi
#define PTR_ADDIU	addiu
#define PTR_SUB		sub
#define PTR_SUBU	subu
#define PTR_L		lw
#define PTR_S		sw
#define PTR_LA		la
#define PTR_SLL		sll
#define PTR_SLLV	sllv
#define PTR_SRL		srl
#define PTR_SRLV	srlv
#define PTR_SRA		sra
#define PTR_SRAV	srav

#define PTR_SCALESHIFT	2

#define PTR		.word
#define PTRSIZE		4
#define PTRLOG		2


/*
 * Some cp0 registers were extended to 64bit for MIPS III.
 */

#define MFC0	mfc0
#define MTC0	mtc0
#define DMFC0	mfc0
#define DMTC0	mtc0
#define LDC1	lwc1
#define SDC1	lwc1

#define SSNOP	sll zero,zero,1
#define NOPS	SSNOP; SSNOP; SSNOP; SSNOP


# Implementation status

This section documents the implementation status for all HP50 RPL commands as
listed in the HP50G Advanced Reference Manual. This is a strict superset of the
HP48 implementation.

* [Implemented](#implemented-commands)
* [Not implemented](#unimplemented-commands)
* [Additional](#additional-commands)


# Implemented commands

The following is a list of the HP50 RPL commands which are implemented in DB48X.

* [!](#fact) (Factorial)
* [+](#add) (Add)
* [<](#testlt) (Less than)
* [==](#same) (Different meaning: object equality)
* [=](#testeq) (Equal)
* [>](#testgt) (Greater than)
* [ABS](#abs)
* [ACOSH](#acosh)
* [ACOS](#acos)
* [ADD](#add)
* [ALOG](#alog)
* [AND](#and)
* [ARG](#arg)
* [ASINH](#asinh)
* [ASIN](#asin)
* [ATANH](#atanh)
* [ATAN](#atan)
* [AXES](#axes)
* [BIN](#bin)
* [BYTES](#bytes)
* [B→R](#binarytoreal)
* [CASE](#case)
* [CF](#clearflag)
* [CLLCD](#cllcd)
* [CLΣ](#cleardata)
* [CONJ](#conj)
* [CONT](#continue)
* [CONVERT](#convert)
* [COSH](#cosh)
* [COS](#cos)
* [CRDIR](#crdir)
* [C→R](#complextoreal)
* [DBUG](#debug)
* [DEC](#dec)
* [DEG](#deg)
* [DEPTH](#depth)
* [DIR](#dir)
* [DISP](#disp)
* [DOERR](#doerr)
* [DO](#do)
* [DRAW](#draw)
* [DRAX](#drax)
* [DROP2](#drop2)
* [DROPN](#dropn)
* [DROP](#drop)
* [DTAG](#dtag)
* [DUP2](#dup2)
* [DUPN](#dupn)
* [DUP](#dup)
* [ELSE](#else)
* [END](#end)
* [ENG](#eng)
* [ERR0](#err0)
* [ERRM](#errm)
* [ERRN](#errn)
* [EVAL](#eval)
* [EXPAND](#expand)
* [EXPAN](#expan)
* [EXPM](#expm)
* [EXP](#exp)
* [FACT](#fact)
* [FC?C](#testflagclearthenclear)
* [FC?](#testflagclear)
* [FIX](#fix)
* [FOR](#for)
* [FS?C](#testflagsetthenclear)
* [FS?](#testflagset)
* [FUNCTION](#function)
* [GAMMA](#gamma)
* [GET](#get)
* [GOR](#gor)
* [GROB](#grob)
* [GXOR](#gxor)
* [HALT](#halt)
* [HELP](#help) (Different meaning)
* [HEX](#hex)
* [HOME](#home)
* [IFERR](#iferr)
* [IFTE](#ifte)
* [IFT](#ift)
* [IF](#if)
* [IM](#im)
* [INV](#inv)
* [KILL](#kill)
* [LASTARG](#lastarg)
* [LINE](#line)
* [LNP1](#lnp1)
* [LN](#ln)
* [LOG](#log)
* [MAXΣ](#maxdata)
* [MEAN](#mean)
* [MEM](#mem)
* [MINΣ](#mindata)
* [MOD](#mod)
* [NEG](#neg)
* [NEXT](#next)
* [NOT](#not)
* [OCT](#oct)
* [OFF](#off)
* [OR](#or)
* [OVER](#over)
* [PARAMETRIC](#parametric)
* [PATH](#path)
* [PGDIR](#pgdir)
* [PICK](#pick)
* [PICT](#pict)
* [POLAR](#polar)
* [PURGE](#purge)
* [RAD](#rad)
* [RCL](#rcl)
* [RCWS](#rcws)
* [RECT](#rect) (Different meaning: draws a rectangle)
* [REPEAT](#repeat)
* [REWRITE](#rewrite) (Different meaning: performs a rewrite)
* [ROLLD](#rolld)
* [ROLL](#roll)
* [ROOT](#root)
* [ROT](#rot)
* [R→B](#realtobinary)
* [R→C](#realtocomplex)
* [SAME](#same)
* [SCI](#sci)
* [SF](#showflag)
* [SIGN](#sign)
* [SINH](#sinh)
* [SIN](#sin)
* [SQ](#sq)
* [SST](#stepover)
* [SST↓](#singlestep)
* [STEP](#step)
* [STORE](#store) (Different meaning: long form of STO)
* [STO](#sto)
* [STWS](#stws)
* [SWAP](#swap)
* [TANH](#tanh)
* [TAN](#tan)
* [THEN](#then)
* [TICKS](#ticks)
* [TYPE](#type)
* [UBASE](#ubase)
* [UFACT](#ufact)
* [UNTIL](#until)
* [UPDIR](#updir)
* [UVAL](#uval)
* [VERSION](#version)
* [WAIT](#wait)
* [WHILE](#while)
* [XCOL](#independentcolumn)
* [XOR](#xor)
* [XROOT](#xroot)
* [YCOL](#dependentcolumn)
* [^](#pow) (Power)
* [i](#iconstant)
* [«»](#programs) (Program delimiters)
* [×](#mul) (Multiply)
* [÷](#div) (Divide)
* [ΣXY](#sumofxy)
* [ΣX](#sumofx)
* [ΣX²](#sumofxsquares)
* [ΣY](#sumofy)
* [ΣY²](#sumofysquares)
* [π](#pi) (Pi)
* [–](#sub) (Subtract)
* [→LIST](#tolist)
* [→NUM](#todecimal)
* [→Q](#tofraction)
* [→STR](#totext)
* [STR→](#compile)
* [→TAG](#→tag)
* [→UNIT](#→unit)
* [→](#locals) (Create Local)
* [√](#sqrt) (Square root)
* [∫](#integrate) (Integrate)
* [≠](#testne) (Not equal)
* [≤](#testle) (Less than or equal)
* [≥](#testge) (Greater than or Equal)
* [＿](#units) (Unit attachment)



# Unimplemented commands

The following is a list of unimplemented HP50 RPL commands, which is a superset of the HP48

commands.

* ABCUV
* ACK
* ACKALL
* ACOS2S
* ADDTMOD
* ADDTOREAL
* ALGB
* AMORT
* ANIMATE
* ANS
* APPLY
* ARC
* ARCHIVE
* ARIT
* ARRY→
* →ARRY
* ASIN2C
* ASIN2T
* ASN
* ASR
* ASSUME
* ATAN2S
* ATICK
* ATTACH
* AUGMENT
* AUTO
* AXL
* AXM
* AXQ
* BAR
* BARPLOT
* BASIS
* BAUD
* BEEP
* BESTFIT
* BINS
* BLANK
* BOX
* BUFLEN
* C$
* C2P
* CASCFG
* CASCMD
* CEIL
* CENTR
* %CH
* CHINREM
* CHOLESKY
* CHOOSE
* CHR
* CIRC
* CKSM
* CLEAR
* CLKADJ
* CLOSEIO
* CLUSR
* CLVAR
* CMPLX
* CNRM
* →COL
* COL→
* COL–
* COL+
* COLCT
* COLLECT
* COLΣ
* COMB
* CON
* COND
* CONIC
* CONLIB
* CONST
* CONSTANTS
* CORR
* COV
* CR
* CROSS
* CSWP
* CURL
* CYCLOTOMIC
* CYLIN
* C→PX
* DARCY
* DATE
* →DATE
* DATE+
* DDAYS
* DECR
* DEDICACE
* DEF
* DEFINE
* DEGREE
* DELALARM
* DELAY
* DELKEYS
* DEPND
* DERIV
* DERVX
* DESOLVE
* DET
* DETACH
* DIAG→
* →DIAG
* DIAGMAP
* DIFF
* DIFFEQ
* DISPXY
* DISTRIB
* DIV
* DIV2
* DIV2MOD
* DIVIS
* DIVMOD
* DIVPC
* dn
* DOLIST
* DOMAIN
* DOSUBS
* DOT
* DRAW3DMATRIX
* DROITE
* DUPDUP
* D→R
* e
* EDIT
* EDITB
* EGCD
* EGV
* EGVL
* ENDSUB
* EPSX0
* EQNLIB
* EQW
* EQ→
* ERASE
* EULER
* EXLR
* EXP&LN
* EXP2HYP
* EXP2POW
* EXPANDMOD
* EXPFIT
* EXPLN
* EYEPT
* F0λ
* FACTOR
* FACTORMOD
* FACTORS
* FANNING
* FAST3D
* FCOEF
* FDISTRIB
* FFT
* FILER
* FINDALARM
* FINISH
* FLASHEVAL
* FLOOR
* FONT6
* FONT7
* FONT8
* FONT→
* →FONT
* FOURIER
* FP
* FREE
* FREEZE
* FROOTS
* FXND
* GAUSS
* GBASIS
* GCD
* GCDMOD
* GETI
* GRAD
* GRAMSCHMIDT
* GRAPH
* GREDUCE
* GRIDMAP
* →GROB
* GROBADD
* *H
* HADAMARD
* HALFTAN
* HEAD
* HEADER→
* →HEADER
* HERMITE
* HESS
* HILBERT
* HISTOGRAM
* HISTPLOT
* HMS–
* HMS+
* HMS→
* →HMS
* HORNER
* IABCUV
* IBASIS
* IBERNOULLI
* IBP
* ICHINREM
* IDN
* IDIV2
* IEGCD
* IFFT
* ILAP
* IMAGE
* INCR
* INDEP
* INFORM
* INPUT
* INT
* INTEGER
* INTVX
* INVMOD
* IP
* IQUOT
* IREMAINDER
* ISOL
* ISOM
* ISPRIME?
* I→R
* JORDAN
* KER
* KERRM
* KEY
* KEYEVAL
* →KEYTIME
* KEYTIME→
* KGET
* LABEL
* LAGRANGE
* LANGUAGE→
* →LANGUAGE
* LAP
* LAPL
* LAST
* LCD→
* →LCD
* LCM
* LCXM
* LDEC
* LEGENDRE
* LGCD
* LIBEVAL
* LIBS
* lim
* LIMIT
* LIN
* ΣLINE
* LINFIT
* LININ
* LINSOLVE
* LIST→
* ∆LIST
* ΠLIST
* ΣLIST
* LNAME
* LNCOLLECT
* LOCAL
* LOGFIT
* LQ
* LR
* LSQ
* LU
* LVAR
* MAD
* MAIN
* MANT
* MAP
* ↓MATCH
* ↑MATCH
* MATHS
* MATR
* MAX
* MAXR
* MCALC
* MENU
* MENUXY
* MERGE
* MIN
* MINEHUNT
* MINIFONT→
* →MINIFONT
* MINIT
* MINR
* MITM
* MKISOM
* MODSTO
* MODULAR
* MOLWT
* MROOT
* MSGBOX
* MSLV
* MSOLVR
* MULTMOD
* MUSER
* →NDISP
* NDIST
* NDUPN
* NEWOB
* NEXTPRIME
* NIP
* NOVAL
* NΣ
* NSUB
* NUM
* NUMX
* NUMY
* OBJ→
* OLDPRT
* OPENIO
* ORDER
* P2C
* PA2B2
* PARITY
* PARSURFACE
* PARTFRAC
* PCAR
* PCOEF
* PCONTOUR
* PCOV
* PDIM
* PERINFO
* PERM
* PERTBL
* PEVAL
* PICK3
* PICTURE
* PINIT
* PIX?
* PIXOFF
* PIXON
* PKT
* PLOT
* PLOTADD
* PMAX
* PMIN
* PMINI
* POLYNOMIAL
* POP
* POS
* POTENTIAL
* POWEXPAND
* POWMOD
* PR1
* PREDV
* PREDX
* PREDY
* PREVAL
* PREVPRIME
* PRLCD
* PROMPT
* PROMPTSTO
* PROOT
* PROPFRAC
* PRST
* PRSTC
* PRVAR
* PSDEV
* PSI
* Psi
* PTAYL
* PTPROP
* PUSH
* PUT
* PUTI
* PVAR
* PVARS
* PVIEW
* PWRFIT
* PX→C
* →Qπ
* qr
* QR
* QUAD
* QUOT
* QUOTE
* QXA
* RAND
* RANK
* RANM
* RATIO
* RCEQ
* RCI
* RCIJ
* RCLALARM
* RCLF
* RCLKEYS
* RCLMENU
* RCLVX
* RCLΣ
* RDM
* RDZ
* RE
* RECN
* RECV
* REF
* REMAINDER
* RENAME
* REORDER
* REPL
* RES
* RESTORE
* RESULTANT
* REVLIST
* RISCH
* RKF
* RKFERR
* RKFSTEP
* RL
* RLB
* RND
* RNRM
* ROMUPLOAD
* ROW–
* ROW+
* ROW→
* →ROW
* RPL>
* RR
* RRB
* rref
* RREF
* RREFMOD
* RRK
* RRKSTEP
* RSBERR
* RSD
* RSWP
* RULES
* R→D
* R→I
* SBRK
* SCALE
* SCALEH
* SCALEW
* SCATRPLOT
* SCATTER
* SCHUR
* SCLΣ
* SCONJ
* SCROLL
* SDEV
* SEND
* SEQ
* SERIES
* SERVER
* SEVAL
* SHOW
* SIDENS
* SIGMA
* SIGMAVX
* SIGNTAB
* SIMP2
* SIMPLIFY
* SINCOS
* SINV
* SIZE
* SL
* SLB
* SLOPEFIELD
* SNEG
* SNRM
* SOLVE
* SOLVEQN
* SOLVER
* SOLVEVX
* SORT
* SPHERE
* SR
* SRAD
* SRB
* SRECV
* SREPL
* START
* STD
* STEQ
* STIME
* STOALARM
* STOF
* STOKEYS
* STOVX
* STO+
* STO–
* STO*
* STO/
* STOΣ
* STREAM
* STRM
* STURM
* STURMAB
* SUB
* SUBST
* SUBTMOD
* SVD
* SVL
* SYSEVAL
* SYLVESTER
* SYST2MAT
* %T
* TABVAL
* TABVAR
* TAIL
* TAN2CS2
* TAN2SC
* TAN2SC2
* TAYLOR0
* TAYLR
* TCHEBYCHEFF
* TCOLLECT
* TDELTA
* TESTS
* TEVAL
* TEXPAND
* TEXT
* TIME
* →TIME
* TINC
* TLIN
* TLINE
* TMENU
* TOT
* TRACE
* TRAN
* TRANSIO
* TRIG
* TRIGCOS
* TRIGO
* TRIGSIN
* TRIGTAN
* TRN
* TRNC
* TRUNC
* TRUTH
* TSIMP
* TSTR
* TVARS
* TVM
* TVMBEG
* TVMEND
* TVMROOT
* UFL1→MINIF
* UNASSIGN
* UNASSUME
* UNBIND
* UNPICK
* UNROT
* UTPC
* UTPF
* UTPN
* UTPT
* V→
* →V2
* →V3
* VANDERMONDE
* VAR
* VARS
* VER
* VISIT
* VISITB
* VPOTENTIAL
* VTYPE
* *W
* WIREFRAME
* WSLOG
* XGET
* XMIT
* XNUM
* XPON
* XPUT
* XQ
* XRECV
* XRNG
* XSEND
* XSERV
* XVOL
* XXRNG
* YRNG
* YSLICE
* YVOL
* YYRNG
* ZEROS
* ZFACTOR
* ZVOL
* | (Where)
* ?
* ∞
* Σ
* Σ+
* Σ–
* ∂
* % (Percent)
*  (Store)
* ; (Semicolon)

## Additional commands

The following commands are unique to DB48X and are not found in any
Hewlett-Packard RPL implementation.

* [ATAN2](#atan2): Arc-tangent from two arguments
* [AngleUnitsMenu](#angleunitsmenu)
* [AnglesMenu](#anglesmenu)
* [ApplyInverseUnit](#applyinverseunit)
* [ApplyUnit](#applyunit)
* [AreaUnitsMenu](#areaunitsmenu)
* [AutoSimplify](#autosimplify): Automatically simplify expressions
* [BASE](#base): Select an arbitrary base for based numbers
* [Background](#background): Select background pattern for graphic operations
* [BasedDotOrComma](#baseddotorcomma): Use dot or comma as based number digit separator
* [BasedSpaces](#basedspaces): Use thin spaces as based number digit separator
* [BasedSpacing](#basedspacing): Grouping of digits for based numbers
* [BasedTicks](#basedticks): Use tick marsk `'` as based number digit separator
* [BasedUnderscore](#basedunderscore): Use underscore `_` as based number digit separator
* [BasesMenu](#basesmenu)
* [CBRT](#cbrt): Cube root
* [CYCLE](#cycle): Cycle between object representations
* [Capitalized](#capitalized): Show commands capitalized
* [Catalog](#catalog): Present catalog of all functions with auto-completion
* [CharsMenu](#charsmenu)
* [CircularMenu](#circularmenu)
* [ClassicExponent](#classicexponent): Use E as exponent marker, e.g. 1.3E128
* [ClearThingsMenu](#clearthingsmenu)
* [CompareMenu](#comparemenu)
* [ComplexMenu](#complexmenu)
* [ComputerUnitsMenu](#computerunitsmenu)
* [ConstantsMenu](#constantsmenu)
* [ConvertToUnitPrefix](#converttounitprefix)
* [ConvertToUnit](#converttounit)
* [CursorBlinkRate](#cursorblinkrate): Select cursor blink rate in milliseconds
* [DebugMenu](#debugmenu)
* [DecimalComma](#decimalcomma): Select comma as decimal separator
* [DecimalDot](#decimaldot): Select dot as decimal separator
* [DifferentialSolverMenu](#differentialsolvermenu)
* [DifferentiationMenu](#differentiationmenu)
* [DisplayModesMenu](#displaymodesmenu)
* [EQUIV](#equiv): Logical equivalence
* [ERFC](#erfc): Complementary error function
* [ERF](#erf): Error function
* [EXCLUDES](#excludes): Logical exclusion
* [EditMenu](#editmenu)
* [EditorBegin](#editorbegin)
* [EditorClear](#editorclear)
* [EditorCopy](#editorcopy)
* [EditorCut](#editorcut)
* [EditorEnd](#editorend)
* [EditorFlip](#editorflip)
* [EditorFontSize](#editorfontsize): Select font size for text editor
* [EditorMultilineFontSize](#editormultilinefontsize): Select font size for multi-line text editor
* [EditorPaste](#editorpaste)
* [EditorReplace](#editorreplace)
* [EditorSearch](#editorsearch)
* [EditorSelect](#editorselect)
* [EditorWordLeft](#editorwordleft)
* [EditorWordRight](#editorwordright)
* [ElectricityUnitsMenu](#electricityunitsmenu)
* [EnergyUnitsMenu](#energyunitsmenu)
* [EquationsMenu](#equationsmenu)
* [ExpLogMenu](#explogmenu)
* [FancyExponent](#fancyexponent): Use power-of-ten rendering, e.g. 1.3×₁₀¹²⁸
* [FilesMenu](#filesmenu)
* [FinanceSolverMenu](#financesolvermenu)
* [FlagsMenu](#flagsmenu)
* [FlatMenus](#flatmenus): Flatten menus (no use of shift)
* [ForceUnitsMenu](#forceunitsmenu)
* [Foreground](#foreground): Select foreground pattern for graphic operations
* [FractionSpacing](#fractionspacing): Grouping of digits for fractional part of numbers
* [FractionsMenu](#fractionsmenu)
* [GAND](#gand): Graphical And
* [GarbageCollect](#garbagecollect)
* [GraphicsMenu](#graphicsmenu)
* [GraphicsStackDisplay](#graphicsstackdisplay): Select graphic display of the stack
* [HYPOT](#hypot): Hypothenuse
* [HideBuiltinUnits](#hidebuiltinunits)
* [HyperbolicMenu](#hyperbolicmenu)
* [IMPLIES](#implies): Logical implication
* [IOMenu](#iomenu)
* [IntegrationMenu](#integrationmenu)
* [LastMenu](#lastmenu): Select last menu
* [LastX](#lastx): Return last X argument (for easier translation of RPN programs)
* [LengthUnitsMenu](#lengthunitsmenu)
* [LibsMenu](#libsmenu)
* [LightUnitsMenu](#lightunitsmenu)
* [LineWidth](#linewidth): Select line width for line drawing operations
* [LinearSolverMenu](#linearsolvermenu)
* [ListMenu](#listmenu)
* [LongForm](#longform): Show commands in long form
* [LoopsMenu](#loopsmenu)
* [LowerCase](#lowercase): Show commands in lowercase
* [MainMenu](#mainmenu)
* [MantissaSpacing](#mantissaspacing): Grouping of digits for whole part of numbers
* [MassUnitsMenu](#massunitsmenu)
* [MathMenu](#mathmenu)
* [MathModesMenu](#mathmodesmenu)
* [MatrixMenu](#matrixmenu)
* [MaxBigNumBits](#maxbignumbits): Maximum number of bits for a big integer
* [MaxRewrites](#maxrewrites): Maximum number of equation rewrites
* [MemMenu](#memmenu)
* [MenuFirstPage](#menufirstpage)
* [MenuNextPage](#menunextpage)
* [MenuPreviousPage](#menupreviouspage)
* [MinimumSignificantDigits](#minimumsignificantdigits): adjustment of FIX mode switch to SCI
* [ModesMenu](#modesmenu)
* [MultiSolverMenu](#multisolvermenu)
* [NAND](#nand): Not And
* [NOR](#nor): Not Or
* [NoAutoSimplify](#noautosimplify): Do not automatically simplify expressions
* [NoTrailingDecimal](#notrailingdecimal): display 1.0 as 1
* [NumberDotOrComma](#numberdotorcomma): Use dot or comma as digit group separator
* [NumberSpaces](#numberspaces): Use thin spaces as digit group separator
* [NumberTicks](#numberticks): Use tick marks `'` as digit group separator
* [NumberUnderscore](#numberunderscore): Use underscore `_` as digit group separator
* [NumbersMenu](#numbersmenu)
* [NumericResults](#numericresults): Produce numeric (decimal) results
* [NumericalSolverMenu](#numericalsolvermenu)
* [ObjectMenu](#objectmenu)
* [PIRADIANS](#piradians): Angle mode with multiples of pi
* [PartsMenu](#partsmenu)
* [PlotMenu](#plotmenu)
* [PolynomialSolverMenu](#polynomialsolvermenu)
* [PolynomialsMenu](#polynomialsmenu)
* [PowerUnitsMenu](#powerunitsmenu)
* [PowersMenu](#powersmenu)
* [Precision](#precision): Select decimal computing precision
* [PressureUnitsMenu](#pressureunitsmenu)
* [PrintingMenu](#printingmenu)
* [ProbabilitiesMenu](#probabilitiesmenu)
* [ProgramMenu](#programmenu)
* [REM](#rem): remainder
* [RadiationUnitsMenu](#radiationunitsmenu)
* [RealMenu](#realmenu)
* [ResultFontSize](#resultfontsize): Select font size to display level 1 of stack
* [RoundedMenus](#roundedmenus): Select round menu style
* [SIG](#sig): Significant digits mode
* [SaveState](#savestate): Save system state to current state file
* [SelfInsert](#selfinsert)
* [SeparatorModesMenu](#separatormodesmenu)
* [ShowBuiltinUnits](#showbuiltinunits)
* [SignalProcessingMenu](#signalprocessingmenu)
* [SingleRowMenus](#singlerowmenus): Display menus on single row
* [SolverMenu](#solvermenu)
* [SpeedUnitsMenu](#speedunitsmenu)
* [SquareMenus](#squaremenus): Select square (C47-like) menu style
* [StackFontSize](#stackfontsize): Select font size to display levels above 1 of stack
* [StackMenu](#stackmenu)
* [StandardExponent](#standardexponent): Display with standard exponent mode
* [StatisticsMenu](#statisticsmenu)
* [SymbolicMenu](#symbolicmenu)
* [SymbolicResults](#symbolicresults): Produce symbolic results
* [SymbolicSolverMenu](#symbolicsolvermenu)
* [SystemMemory](#systemmemory)
* [SystemSetup](#systemsetup): Enter DMCP system setup menu
* [Tag](#tag)→
* [TemperatureUnitsMenu](#temperatureunitsmenu)
* [TestsMenu](#testsmenu)
* [TextMenu](#textmenu)
* [TextStackDisplay](#textstackdisplay): Select text-only display of the stack
* [ThreeRowsMenus](#threerowsmenus): Display menus on up to three rows
* [TimeMenu](#timemenu)
* [TimeUnitsMenu](#timeunitsmenu)
* [ToFractionDigits](#tofractiondigits): Required digits of precision for →Q
* [ToFractionIterations](#tofractioniterations): Max number of iterations for →Q
* [ToolsMenu](#toolsmenu): Automatically select a menu based on context
* [TrailingDecimal](#trailingdecimal): display 1.0 with trailing decimal separator
* [TypeName](#typename)
* [Undo](#undo): Restore stack to state before command
* [UnitsConversionsMenu](#unitsconversionsmenu)
* [UnitsMenu](#unitsmenu)
* [UpperCase](#uppercase): Show commands in uppercase
* [UserInterfaceModesMenu](#userinterfacemodesmenu)
* [VariablesMenuExecute](#variablesmenuexecute)
* [VariablesMenuRecall](#variablesmenurecall)
* [VariablesMenuStore](#variablesmenustore)
* [VariablesMenu](#variablesmenu)
* [VectorMenu](#vectormenu)
* [ViscosityUnitsMenu](#viscosityunitsmenu)
* [VolumeUnitsMenu](#volumeunitsmenu)

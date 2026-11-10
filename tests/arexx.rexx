/* arexx.rexx: Code::Blocks's ARexx port (CODEBLOCKS.1). Start Code::Blocks first, with
 * RexxMast running. Prints PASS or FAIL for each check, then QUITs it and
 * checks that the port has gone. rx tests/arexx.rexx
 * Copyright (c) 2026 AmiCradle Team */
OPTIONS RESULTS
PARSE ARG dummy
bad = 0
IF ~SHOW('P', 'CODEBLOCKS.1') THEN DO
    SAY 'FAIL Code::Blocks is not running (no CODEBLOCKS.1)'
    EXIT 20
END
ADDRESS 'CODEBLOCKS.1'

'VERSION'
CALL check 'VERSION', rc = 0 & POS('Code::Blocks ', result) = 1
'HELP'
CALL check 'HELP', rc = 0 & POS('QUIT', result) > 0 & POS('OPEN', result) > 0 & POS('SAVEALL', result) > 0 & POS('BUILD', result) > 0 & POS('BUSY', result) > 0
'FAULT 205'
CALL check 'FAULT', rc = 0 & result ~= ''
'NOSUCH'
CALL check 'unknown command', rc = 20
'DEACTIVATE'
CALL check 'DEACTIVATE', rc = 0
'ACTIVATE'
CALL check 'ACTIVATE', rc = 0
'BUSY'
CALL check 'BUSY', rc = 0 & result = 0
'OPEN NoSuchDrawer/none.cbp'
CALL check 'OPEN missing', rc = 10
'BUILD'
CALL check 'BUILD with no project', rc = 10
'QUIT'
CALL check 'QUIT', rc = 0
DO i = 1 TO 30
    IF ~SHOW('P', 'CODEBLOCKS.1') THEN LEAVE
    ADDRESS COMMAND 'Wait 1'
END
CALL check 'port gone after QUIT', ~SHOW('P', 'CODEBLOCKS.1')
IF bad = 0 THEN SAY 'codeblocks arexx: ALL PASS'
ELSE SAY 'codeblocks arexx:' bad 'FAILED'
EXIT bad

check: PARSE ARG what, ok
IF ok THEN SAY 'PASS' what
ELSE DO
    SAY 'FAIL' what '(rc' rc')'
    bad = bad + 1
END
RETURN

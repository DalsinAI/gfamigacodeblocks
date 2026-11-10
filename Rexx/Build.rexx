/* Build.rexx: Code::Blocks builds the open project and waits for it.
   rx Build.rexx [TARGET]   (Code::Blocks running, a project open) */
OPTIONS RESULTS
PARSE ARG target
IF ~SHOW('P', 'CODEBLOCKS.1') THEN DO
    SAY 'Code::Blocks is not running.'
    EXIT 5
END
ADDRESS 'CODEBLOCKS.1'
'SAVEALL'
IF target = '' THEN 'BUILD WAIT'
ELSE 'BUILD' '"'target'"' 'WAIT'
IF rc = 0 THEN SAY 'Built.'
ELSE SAY 'The build did not finish (rc' rc')'

#!/usr/bin/perl

#
# Regression test script for vscript compiler.
#

use strict;

###
# Parameters
###
my $TEST_PROG;
my $TEST_DIR  = "tst";
my $RESULT_DIR = "out"; 
my $VM_TRACE = 1;
my $MAGIC_WINDOWS_STATUS = 1020;
my $IS_UNIX=0;


sub init()
{
    if ($^O eq "MSWin32") {
        $TEST_PROG="..\\..\\win32\\debug\\vscripttest.exe";
        #$TEST_PROG="..\\..\\win32\\release\\vscripttest.exe";
     } elsif ($^O eq "linux") {
	$TEST_PROG=$ENV{"ROOT_DIR"}."/linux/bin/vscripttest";
	$IS_UNIX=1;
    } else {
	print("unsupported system");
	exit(1);
    }
}

###
# Test result counters
###

my $RUN_OK = 0;
my $TEST_FAILED = 0;

my $AST_ERROR = 0;
my $LST_ERROR = 0;
my $LST2_ERROR = 0;
my $STDOUT_ERROR = 0;
my $STDERR_ERROR = 0;
my $TEST_FAILED = 0;




###
# show test message
###
sub showMessage
{
  my $msg = shift;
  my $len = length($msg) + 2;
  my $pos;

  print $msg."  ";
  for( ;$len<44;$len+=1) {
    print ".";
  }  
  print " ";
}

###
# show error message
###
sub showError
{
  my $message = join( " ", @_);
 
  print("[Failed]\n\n$message\n\n");
	
 #print "\n*** see output files ${file_stderr} ${file_stdout} ${file_ast}\n\ 	
}

# map compiler status to message string
my %CL_STATUS = {
	0 => "ok",
	1 => "NO_INPUT_ERROR",
	2 => "INTERNAL_ERROR",
	
	3 => "CL_NO_INPUT_ERROR",
	    
	4 => "CL_SYNTAX_ERROR",
	5 => "CL_SEMANTIC_OR_CODEGEN_ERROR",
	6 => "CL_BACKREF_ERROR",
	7 => "CL_WRITE_EXE_ERROR",
     
	8 =>  "XMETHOD_WRONG_PARAMETER_SPEC",
	9 =>  "XMETHOD_ID_REPEATED",
	10 => "XMETHOD_ALREADY_DEFINED",

	11 => "VM_RUNTIME_ERROR",
	12 => "VM_SUSPENDED_FOR_XMETHOD",
};



sub DoDiff
{
   
   my $file_actual   =  $_[1];
   my $file_expected =  $_[0]; 

   if (! -f $file_actual && ! -f $file_expected) {
      return 0;
   }

   system("unix2dos $file_actual 2>/dev/null") if ($IS_UNIX != 0);

   return `diff -ni $file_actual $file_expected`;

}

###
# Check compilation 
###
sub CheckFile
{
  my $file = shift;
  my $expected = shift;
  my $res;
  my $out;
  
  my $file_src    =  "${TEST_DIR}/${file}";
  my $file_stderr =  "${RESULT_DIR}/${file}.err"; 
  my $file_stdout =  "${RESULT_DIR}/${file}.out"; 
  my $file_ast    =  "${RESULT_DIR}/${file}.xml"; 
  my $file_lst    =  "${RESULT_DIR}/${file}.lst"; 
  my $file_lst2   =  "${RESULT_DIR}/${file}.lst2"; 


  my $file_expected_ast =  "${TEST_DIR}/${file}.xml"; 
  my $file_expected_lst =  "${TEST_DIR}/${file}.lst"; 
  my $file_expected_lst2 = "${TEST_DIR}/${file}.lst2"; 
  my $file_expected_stdout = "${TEST_DIR}/${file}.out"; 
  my $file_expected_stderr = "${TEST_DIR}/${file}.err"; 
  my $passed;

  my $cmd = "$TEST_PROG ${file_src} ${RESULT_DIR} ${VM_TRACE} 2>${file_stderr} >${file_stdout}";

  #print "$cmd\n";
 
  showMessage("Testing: $file");

  $out=`$cmd`;
  $res = $?;

  
  if ($expected != $res) {
    my $exp_name;
    my $res_name;
    
    $expected -= $MAGIC_WINDOWS_STATUS  if ($expected > $MAGIC_WINDOWS_STATUS); 
    $res -= $MAGIC_WINDOWS_STATUS  if ($res > $MAGIC_WINDOWS_STATUS);

    #$exp_name = defined($CL_STATUS{$expected}) ? $CL_STATUS{$expected} : " status: $expected" ;
    #$res_name = defined($CL_STATUS{$res}) ? $CL_STATUS{$res} : " status: $res" ;
    $exp_name = " status: $expected ".$CL_STATUS{$expected} ;
    $res_name = " status: $res".$CL_STATUS{$res} ;



    showError("expected $exp_name but actual result $res_name");

    $TEST_FAILED += 1;
 
    return;
  }

  if (-f ${file_expected_ast}) {
     $passed = "$passed/ast";
 
     #`diff -ni ${file_expected_ast} ${file_ast}`;
     #if ($? != 0) {

     if (DoDiff( $file_expected_ast, $file_ast )) {
	showError("expected AST ${file_expected_ast} differs from recorded AST ${file_ast}");

	$AST_ERROR += 1;
	$TEST_FAILED += 1;

        return;
     }
  }   

  if (-f ${file_expected_lst}) {
     $passed = "$passed/lst";
     
     #`diff -ni ${file_expected_lst} ${file_lst}`;
     #if ($? != 0) {

     if (DoDiff( $file_expected_lst, $file_lst )) {
	showError("expected Listing ${file_expected_lst} differs from recorded Listing ${file_lst}");
	
	$LST_ERROR += 1;
	$TEST_FAILED += 1;

        return;
     }
  }   

  if (-f ${file_expected_lst2}) {
     $passed = "$passed/lst2";
     
     #`diff -ni ${file_expected_lst2} ${file_lst2}`;     
     #if ($? != 0) {

     if (DoDiff( $file_expected_lst2, $file_lst2)) {
	showError("expected resolved Listing ${file_expected_lst2} differs from recorded resolved Listing ${file_lst2}");
	
	$LST2_ERROR += 1;
	$TEST_FAILED += 1;

        return;
     }
  }   
  
  if (-f ${file_expected_stdout}) {
     $passed = "$passed/stdout";

     #`diff -ni ${file_expected_stdout} ${file_stdout}`;
     #if ($? != 0) {

     if (DoDiff( $file_expected_stdout, $file_stdout )) {
	showError("expected standard output ${file_expected_stdout} differs from recorded resolved Listing ${file_stdout}");
	
	$STDOUT_ERROR += 1;
	$TEST_FAILED += 1;

        return;
     }
  }   
    
  if (-f ${file_expected_stderr}) {
     $passed = "$passed/stderr";

     #`diff -ni ${file_expected_stderr} ${file_stderr}`;
     #if ($? != 0) {

     if (DoDiff( $file_expected_stderr, $file_stderr)) {
	showError("expected standard output ${file_expected_stderr} differs from recorded resolved Listing ${file_stderr}");
	
	$STDERR_ERROR += 1;
	$TEST_FAILED += 1;

        return;
     }
  }   
    
  print "[Passed] ${passed}\n";
  $RUN_OK += 1;
}

###
# Check syntax of file and expect test to run ok
###
sub CheckOk
{
  my $file = shift;
  CheckFile($file, 0);
}

###
# Check file and expect syntax check to fail
###
sub SyntaxCheckFail
{
  my $file = shift;
  CheckFile($file, $MAGIC_WINDOWS_STATUS + 3);
}

sub CodeGenCheckFail
{
  my $file = shift;
  CheckFile($file, $MAGIC_WINDOWS_STATUS + 4);
}
 
 
###
# clean temporary results.
###
sub clean_results  
{
   if (-d  ${RESULT_DIR}) {
     system("rm -rf ${RESULT_DIR}");
   }
   if (! -d  ${RESULT_DIR}) {
     system("mkdir -p ${RESULT_DIR}"); 
   }
}

###
# regression test for scripting language.
###
sub syntax_test
{
    print("\n");


    CodeGenCheckFail("err_assign.v");
    CodeGenCheckFail("err_label.v");
    CodeGenCheckFail("err_loop.v");
    CodeGenCheckFail("err_shadow.v");
    CodeGenCheckFail("err_undef_func.v");


    CheckOk("tst-0.v");
    CheckOk("tst.v");
    CheckOk("tst-1.v");
    CheckOk("tst-2.v");
    CheckOk("tst-3.v");
    CheckOk("tst-3a.v");
    CheckOk("tst-4.v");
    CheckOk("tst-5.v");
    CheckOk("tst-5a.v");
    CheckOk("tst-6.v");
    CheckOk("tst-7.v");
    CheckOk("tst-8.v");
    CheckOk("tst-9.v");
    CheckOk("tst-10.v");
    CheckOk("tst-11.v");
    CheckOk("tst-12.v");




}

sub print_results
{
    if ( $TEST_FAILED  > 0) {

	print <<STATUS

*** there were errors ****

Tests passed:	    $RUN_OK 
Tests failed:	    $TEST_FAILED 

AST mismatch:	    $AST_ERROR
LST mismatch:	    $LST_ERROR
LST2 mismatch:	    $LST2_ERROR 
STDOUT mismatch:    $STDOUT_ERROR
STDERR mismatch:    $STDERR_ERROR

STATUS
;
	exit(1);
    } else {
	print <<STATUS

*** all tests ok ***

Tests passed:	    $RUN_OK 
STATUS
;
    }


}

init;
clean_results;
syntax_test;
print_results;


exit( $TEST_FAILED );  



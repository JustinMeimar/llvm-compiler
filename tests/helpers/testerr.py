import subprocess
import os
import sys
import io

"""
This Python program should directly read from .test files and compile them, then run the 
Gazprea programs with LD_PRELOAD set to libgazrt.so and see the program throws an error 
in which step.
Three different results for each Gazprea program: compile error, runtime error or no error
"""

def getAllTestsInDirectory(prefix):
    # return a pair (path, filename) for each test found
    results = []
    for file in os.listdir(prefix):
        full_path = prefix + file
        if os.path.isdir(full_path):
            dir_results = getAllTestsInDirectory(full_path + "/")
            for dir_result in dir_results:
                results.append(dir_result)
        else:
            results.append((full_path, file))
    return results

def run_program(args, inFile = None):
    for arg in args:
        print(arg, end = " ")
    print(flush = True)
    if inFile != None:
        return subprocess.check_output(args, stdin=inFile, timeout=8)
    else:
        return subprocess.check_output(args, timeout=8)

def splitTestFromFile(test_file):
    text = test_file.read()
    results = text.split("#split_token\n")
    for result in results:
        if result.endswith("#split_token"):
            raise RuntimeError("ERROR: a section of input ends with '#split_token' instead of '#split_token\n', did you forget to put \n at the end?")
    return results

def main():

    root_path = "../../"
    test_prefix = root_path + "tests/helpers/error-reporting/"
    libgazrt_path = root_path + "bin/libgazrt.so"

    os.environ["LD_PRELOAD"] = libgazrt_path

    test_in_paths = getAllTestsInDirectory(test_prefix)
    
    selected_tests = [arg[:-5] for arg in sys.argv if arg.endswith(".test")]

    summary_stats = [0, 0]
    failed_files = []

    for test in test_in_paths:
        test_path, test_name = test
        stripped_test_name = test_name[:-5]

        if len(selected_tests) >= 1:
            selected = False
            for selected_test in selected_tests:
                if selected_test == stripped_test_name:
                    selected = True
            if not selected:
                continue

        print("\n\n\ntesting file:" + test_path, file=sys.stderr, flush=True)
        summary_stats[1] += 1

        with open(test_path, "r") as test_file:
            results = splitTestFromFile(test_file)
        if len(results) != 3:
            raise RuntimeError("ERROR: Invalid number of #split_token found in file " + test_path)

        # write the input to a file so gazc can compile it
        with open("../gazprea_program.in", "w") as test_in:
            test_in.write(results[0])

        state_to_name = {
            0: "compile_error",
            1: "llc_error",
            2: "clang_error",
            3: "runtime_error",
            4: "no_error",
            5: "command_died",
        }
        name_to_state = {}
        for state in state_to_name.keys():
            name_to_state[state_to_name[state]] = state
        
        # expected output is translated to expected error state
        expected_error_state = name_to_state[results[2].lower()]
        if expected_error_state == None:
            raise RuntimeError("ERROR: The expected output is not one of the following" +\
                "\ncompile_error\nruntime_error\nno_error")

        error_state = 0
        error_msg = ""
        try:
            # to .ll
            llFile = "../gazprea_program.ll"
            args = [root_path + "bin/gazc", "../gazprea_program.in", llFile]
            run_program(args)

            error_state = 1

            # to .o
            oFile = "../gazprea_program.o"
            args = ["llc", "-filetype=obj", llFile, "-o", oFile]
            run_program(args)

            error_state = 2

            # to binary
            binaryFile = "../gazprea_program"
            args = ["clang", oFile, libgazrt_path, "-o", binaryFile]
            run_program(args)

            error_state = 3

            # run program
            args = [binaryFile]
            with open("../gazprea_program.ins", "w") as inFile:
                inFile.write(results[1])
            with open("../gazprea_program.ins", "r") as inFile:
                print(run_program(args, inFile).decode("UTF-8"), file=sys.stderr, flush=True)

            error_state = 4

        except subprocess.CalledProcessError as e:
            estr = str(e)
            print(estr, file=sys.stderr, flush=True)
            if (estr.find("died with") != -1):
                error_state = 5
        
        if error_state == expected_error_state:
            print("PASS", file=sys.stderr, flush=True)
            summary_stats[0] += 1
        else:
            print("FAILED", file=sys.stderr, flush=True)
            print("expected " + state_to_name[expected_error_state] +\
                 " but got " + state_to_name[error_state], file=sys.stderr, flush=True)
            failed_files.append(test_name)
    
    print("\n\n\npass rate: " + str(summary_stats[0]) + "/" + str(summary_stats[1]), file=sys.stderr, flush=True)
    if (len(failed_files) != 0):
        print("\nfailed tests:", file=sys.stderr, flush=True)
        for failed_file_name in failed_files:
            print(" - " + failed_file_name, file=sys.stderr, flush=True)

if __name__ == "__main__":
    main()
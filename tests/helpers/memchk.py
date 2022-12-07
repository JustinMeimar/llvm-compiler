import subprocess
import os
import time
import sys

def run_program(args, inFile = None):
    for arg in args:
        print(arg, end = " ")
    print(flush = True)
    if inFile != None:
        return subprocess.check_output(args, stdin=inFile, timeout=8)
    else:
        return subprocess.check_output(args, timeout=8)

def fetch_path_file_pair(dir):
    pairs = []
    for tests_dir in os.listdir(dir):
        for filename in os.listdir(dir + tests_dir):
            if (filename.endswith(".in")):
                path = dir + tests_dir + "/" + filename
                pairs.append( (path, tests_dir, filename) )
    return pairs

def main():

    root_path = "../../"
    testin_prefix = root_path + "tests/input/"
    testins_prefix = root_path + "tests/inStream/"
    testout_prefix = root_path + "tests/output/"
    libgazrt_path = root_path + "bin/libgazrt.so"

    os.environ["LD_PRELOAD"] = libgazrt_path

    test_in_paths = fetch_path_file_pair(testin_prefix)
    
    selected_tests = [arg[:-5] for arg in sys.argv if arg.endswith(".test")]

    for test in test_in_paths:
        test_path, tests_dir, test_name = test
        stripped_test_name = test_name[:-3]
        test_ins_path = testins_prefix + tests_dir + "/" + stripped_test_name + ".ins"
        test_out_path = testout_prefix + tests_dir + "/" + stripped_test_name + ".out"
        if (not (os.path.isfile(test_ins_path) and os.path.isfile(test_out_path))):
            continue

        if len(selected_tests) >= 1:
            selected = False
            for selected_test in selected_tests:
                if selected_test == stripped_test_name:
                    selected = True
            if not selected:
                continue
        
        print("\n\n\ntesting file:" + test_path, file=sys.stderr, flush=True)

        try:
            # to .ll
            llFile = "../gazprea_program.ll"
            args = [root_path + "bin/gazc", test_path, llFile]
            if "-gazc" in sys.argv:
                args.insert(0, "valgrind")
            run_program(args)

            # to .o
            oFile = "../gazprea_program.o"
            args = ["llc", "-filetype=obj", llFile, "-o", oFile]
            run_program(args)

            # to binary
            binaryFile = "../gazprea_program"
            args = ["clang", oFile, libgazrt_path, "-o", binaryFile]
            run_program(args)

            # run program
            args = ["valgrind", "--leak-check=yes", binaryFile]
            with open(test_ins_path, "r") as inFile:
                print(run_program(args, inFile).decode("ascii", 'ignore'), file=sys.stderr, flush=True)

        except subprocess.CalledProcessError as e:
            print(str(e), file=sys.stderr, flush=True)

if __name__ == "__main__":
    main()
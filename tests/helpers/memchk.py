import subprocess
import os
import time
import sys

def run_program(args):
    for arg in args:
        print(arg, end = " ")
    print(flush = True)
    return subprocess.check_output(args)

def fetch_path_file_pair(dir):
    pairs = []
    for tests_dir in os.listdir(dir):
        for filename in os.listdir(dir + tests_dir):
            if (filename.endswith(".in")):
                path = dir + tests_dir + "/" + filename
                pairs.append( (path, filename) )
    return pairs

def main():

    root_path = "../../"
    testin_prefix = root_path + "tests/input/"
    libgazrt_path = root_path + "bin/libgazrt.so"

    os.environ["LD_PRELOAD"] = libgazrt_path

    test_in_paths = fetch_path_file_pair(testin_prefix)
    
    selected_tests = [arg[:-5] for arg in sys.argv if arg.endswith(".test")]

    for test in test_in_paths:
        test_path, test_name = test
        if len(selected_tests) >= 1:
            selected = False
            for selected_test in selected_tests:
                if selected_test + ".in" == test_name:
                    selected = True
            if not selected:
                continue
        
        print("\n\n\ntesting file:" + test_path, file=sys.stderr, flush=True)

        try:
            # to .ll
            llFile = "../gazprea_program.ll"
            args = [root_path + "bin/gazc", test_path, llFile]
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
            args = ["valgrind", binaryFile]
            print(run_program(args).decode("UTF-8"), file=sys.stderr, flush=True)

            time.sleep(0.1)
        except subprocess.CalledProcessError as e:
            print(str(e), file=sys.stderr, flush=True)

if __name__ == "__main__":
    main()
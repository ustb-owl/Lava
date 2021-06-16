#! /usr/bin/python3

import os
import sys
import argparse
import subprocess

# directories that storing test cases
dirs = [
    # './cases',
    # 'sysyruntimelibrary/section1/functional_test',
    # 'sysyruntimelibrary/section1/performance_test',
    # 'sysyruntimelibrary/section2/functional_test',
    # 'sysyruntimelibrary/section2/performance_test',
    './testcases/cases'
]

# init compiler config
lacc = "./lacc"
dd = "lli"

# temporary generated executable
exe = 'temp'
# temporary generated IR
ir = '1.ll'

# sysy lib
sylib = "sysy.c"


def info(dataType, data):
    print('\033[036m[+]{0}\033[0m: \033[035m{1}\033[0m'.format(dataType, data))
    return data


# print to stderr
def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)
    sys.stderr.flush()


def GetLacc(path):
    if not os.path.exists(path):
        info("Error", "lacc don't exists");
        exit(1)
    else:
        cmd = "cp " + path + " ./"
        os.system(cmd)


def CompileLibIR():
    cmd = "clang -emit-llvm -S " + sylib
    os.system(cmd)


# scan & collect test cases
def scan_cases(dirs):
    cases = []
    # scan directories
    for i in dirs:
        for root, _, files in os.walk(i):
            for f in sorted(files):
                # find all '*.sy' files
                if f.endswith('.sy'):
                    sy_file = os.path.join(root, f)
                    # add to list of cases
                    cases.append(get_case(sy_file))
    return cases


# get single test case by '*.sy' file
def get_case(sy_file):
    in_file = f'{sy_file[:-3]}.in'
    out_file = f'{sy_file[:-3]}.out'
    if not os.path.exists(in_file):
        in_file = None
    return sy_file, in_file, out_file


# run single test case
def run_ir_case(sy_file, in_file, out_file):
    # compile to executable
    lacc_cmd = lacc.split(' ') + [sy_file]
    result = subprocess.run(lacc_cmd, stdout=subprocess.PIPE)

    if result.returncode:
        return False

    # save output ir file
    with open(ir, 'w+') as f:
        f.write(result.stdout.decode("utf-8").strip())
        f.close()

    # run compiled file
    if in_file:
        with open(in_file) as f:
            inputs = f.read().encode('utf-8')
    else:
        inputs = None

    # link output file with sysy.c IR
    link_cmd = ["llvm-link", '-S', "1.ll", "sysy.ll", "-o", "1.ll"]
    res = subprocess.run(link_cmd)
    if res.returncode:
        return False

    result = subprocess.run(['lli', '1.ll'], input=inputs, stdout=subprocess.PIPE)
    out = f'{result.stdout.decode("utf-8").strip()}\n{result.returncode}'
    out = out.strip()

    # remove temporary file
    # if os.path.exists(ir):
    #     os.unlink(ir)

    # compare to reference
    with open(out_file) as f:
        ref = f.read().strip()
    return out == ref


# run all test cases
def run_ir_test(cases):
    total = 0
    passed = 0
    try:
        for sy_file, in_file, out_file in cases:
            # run test case
            eprint(f'running test "{sy_file}" ... ', end='')
            if run_ir_case(sy_file, in_file, out_file):
                eprint(f'\033[0;32mPASS\033[0m')
                passed += 1
            else:
                eprint(f'\033[0;31mFAIL\033[0m')
            total += 1
    except KeyboardInterrupt:
        eprint(f'\033[0;33mINTERRUPT\033[0m')
    except Exception as e:
        eprint(f'\033[0;31mERROR\033[0m')
        eprint(e)
        exit(1)
    # remove temporary file
    if os.path.exists(exe):
        os.unlink(exe)
    # print result
    if passed == total:
        eprint(f'\033[0;32mPASS\033[0m ({passed}/{total})')
    else:
        eprint(f'\033[0;31mFAIL\033[0m ({passed}/{total})')


# def Compile(file):

if __name__ == '__main__':
    # initialize argument parser
    parser = argparse.ArgumentParser()
    parser.formatter_class = argparse.RawTextHelpFormatter
    parser.description = 'An auto-test tool for MimiC project.'
    parser.add_argument('-i', '--input', default='',
                        help='specify input SysY source file, ' +
                             'default to empty, that means run ' +
                             'files in script configuration')
    parser.add_argument('-d', '--dir', default='',
                        help='specify input SysY source files directory, ' +
                             'default to ./cases')

    parser.add_argument('-l', '--location',
                        default='../cmake-build-debug/lacc',
                        help='specify location of lacc')
    # parse arguments
    args = parser.parse_args()

    # copy lacc
    print(args.location)
    GetLacc(args.location)
    CompileLibIR()

    # start running
    if args.input:
        # check if input test cast is valid
        if not args.input.endswith('.sy'):
            eprint('input must be a SysY source file')
            exit(1)
        if not os.path.exists(args.input):
            eprint(f'file "{args.input}" does not exist')
            exit(1)
        # get absolute path & change cwd
        sy_file = os.path.abspath(args.input)
        os.chdir(os.path.dirname(os.path.realpath(__file__)))
        # get test case
        case = get_case(sy_file)
        if not os.path.exists(case[2]):
            eprint(f'output file "{case[2]}" does not exist')
            exit(1)
        # run test case
        run_ir_test([case])
    else:
        if args.dir:
            os.chdir(os.path.dirname(args.dir))
        else:
            # change cwd to script path
            os.chdir(os.path.dirname(os.path.realpath(__file__)))

        cases = scan_cases(dirs)
        # run test cases in configuration
        # run_test(scan_cases(dirs))
        # info("cases", cases)
        run_ir_test(cases)

#!/usr/bin/python3
import shutil
import os
import argparse
import subprocess

available_suites = [
    "dce-ntimed",
]
# WITH_GDB=0
NS_LOG = ""
# NS_LOG += "***"
# """
NS_LOG += "*=error|warn|prefix_node|prefix_func"

"""
DCE logs
"""
NS_LOG += ":DceCmsg"
NS_LOG += ":DceManager"
NS_LOG += ":DceExecUtils"
NS_LOG += ":DceApplication"
NS_LOG += ":SimuSignal"
NS_LOG += ":SimuFd"
NS_LOG += ":Dce"
NS_LOG += ":ProcessUtils"
NS_LOG += ":DceNetdb"
NS_LOG += ":DceTime"
NS_LOG += ":UnixFileFd"
NS_LOG += ":KernelSocketFd"
NS_LOG += ":UnixDatagramSocketFd"
NS_LOG += ":LinuxSocketImpl"
NS_LOG += ":Ns3SocketFdFactory"
NS_LOG += ":DceNetlinkSocket:DceNetlinkSocketFactory"
NS_LOG += ":NetlinkMessage"
NS_LOG += ":UnixFd"
NS_LOG += ":PcapFile"
#NS_LOG += ":PcapFileHeader"
#NS_LOG += ":PcapFileWriter"

#NS_LOG = "*"
# to see the GetWallTime()
#NS_LOG += ":Node"
#NS_LOG += ":ClockPerfect=*"


# NS_LOG += ":PointToPointNetDevice"
# NS_LOG += ":PointToPointChannel"
# NS_LOG += ":DropTailQueue"
#NS_LOG += ":MpTcpMultiSuite"
#NS_LOG += ":MpTcpTestSuite"
#NS_LOG += ":RandomVariableStream"
#NS_LOG += ":TcpSocketBase"
#NS_LOG += ":TcpNewReno"
#NS_LOG += ":MpTcpSchedulerRoundRobin"
# NS_LOG += ":SimpleNetDevice"
# NS_LOG += ":ObjectBase=error|warn"
#NS_LOG += ":Socket"
# logic:error:warn
#NS_LOG += ":MpTcpSubflow:MpTcpSocketBase"
#NS_LOG += ":MpTcpSubflowUncoupled"
# NS_LOG += ":Config"
# NS_LOG += ":TypeId" # to look for AddTraceSource
#NS_LOG += ":TcpTestSuite"
#NS_LOG += ":TcpRxBuffer"
# NS_LOG += ":PcapFile"
#NS_LOG += ":TcpTxBuffer"
#NS_LOG += ":MpTcpMapping"
# NS_LOG += ":TcpHeader=*"
# NS_LOG=":TcpOptionMpTcp=*"
# NS_LOG=":MpTcpOptionsTestSuite=*"
#NS_LOG += ":TcpL4Protocol"
#NS_LOG += ":Ipv4EndPoint"
# NS_LOG += ":Ipv4EndPointDemux"
# NS_LOG += ":TraceHelper:PointToPointHelper"
# OUTPUT_FILENAME="xp.txt"
# NS_LOG=":MpTcpTestSuite=*|prefix_func:Socket=*"
# """

# type=argparse.FileType('w'),
parser = argparse.ArgumentParser(description="Helper to debug mptcp")

parser.add_argument("suite", choices=available_suites, help="Launch gdb")
parser.add_argument("--debug", '-d', action="store_true", help="Launch gdb")
parser.add_argument("--out", "-o", default="", nargs='?', help="redirect ns3 results output to a file")
parser.add_argument("--verbose", "-v", action="store_const", default="", const="--verbose", help="to enable more output")
parser.add_argument("--graph", "-g", action="store_true", help="Convert pcap to sqlite db and then plot")

args = parser.parse_args()

timeout = None

if args.debug:
    autorun = " -ex 'run  {verbose} {tofile}'"
    cmd = "./waf --run {{suite}} --command-template=\"gdb {autorun} --args %s \" ".format(
            autorun="",
        )
else:
    timeout = 200
    cmd = "./waf --run \"{suite}  {verbose} \" {tofile}"


tofile = " > %s 2>&1" % args.out if args.out else ""
# tofile = " > xp.txt 2>&1"

cmd = cmd.format(
    suite=args.suite,
    verbose=args.verbose,
    # out=
    tofile=tofile,
    fullness="QUICK",
)


os.environ['NS_LOG'] = NS_LOG
#os.environ['DCE_ROOT'] = '/'
os.environ['DCE_ROOT'] = '/'
os.environ['DCE_PATH'] = ''



# we first need to copy the file
os.makedirs("files-1/tmp", exist_ok=True)
shutil.copyfile("/home/teto/dce/ntp.conf", "files-1/tmp/ntp.conf")

try:
    #os.system("echo DCE_ROOT=$DCE_ROOT")
    #ret = True
    ret = subprocess.call(cmd, shell=True, timeout=timeout if timeout else None)
except subprocess.TimeoutExpired:
    print("Timeout expired. try setting a longer timeout")
finally:
    # will be done whatever the results
    pass

print("Exported:\n%s" % NS_LOG)
print("Executed Command:\n%s" % cmd)

if ret:
    print("ERROR: command returned error code %d" % (ret))
    # os.system("truncate --size=100000 %s" % (args.out,))
    exit(1)


def print_result(folder):
    #os.system("echo '========= STDOUT' && cat %s/stdout" % (folder))
    #os.system("echo '========= STDERR' && cat %s/stderr" % (folder))
    #os.system("echo '========= SYSLOG' && cat %s/syslog" % (folder))
    os.system("./dump_folder.sh %s" % (folder,))

if args.graph:
    print("\n\n=== Outputs of node 0 (client) ===")
    print_result("~/dce/files-0/var/log/*")

    print("\n\n=== Outputs of node 1 (server) ===")
    print_result("~/dce/files-1/var/log/2022")

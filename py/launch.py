#!/usr/bin/env python

import resource
import datetime
import subprocess
import sys
import os
import shutil
import pickle
import copy

""" Before starting anything : setting RAM-Limit """
limit = os.environ.get("RAM_LIMIT", 75)*1024*1024*1024
resource.setrlimit(resource.RLIMIT_AS, (limit, limit))
print("Set RAM-Limit to " + str(limit) + "kb!")

"""
Returns a string with todays date+time.
"""
def today():
    return datetime.datetime.now().isoformat()

"""
Returns the time needed by all spawned processes.
"""
def time_needed_by_spawned_processes():
    return resource.getrusage(resource.RUSAGE_CHILDREN)

"""
Collectioned data about run.
"""
class statistics:
    user_time = 0.0
    system_time = 0.0

"""
Contains all parameters.
"""
class config:
    def __init__(self, exec_path, input_file="", current_saft=""):
        self.stats = statistics()
        self.max_columns = os.environ.get("MAX_COLUMNS", 400)
        self.current_saft = current_saft
        self.slave_stop = os.environ.get("SLAVESTOP", None)
        self.extra_args = os.environ.get("EXTRA_ARGS", "")
        self.exec_path = exec_path
        self.input_file = input_file

    """
    Returns the a key that can identify this run. Used later for taking the mean of the same config-runs.
    """
    def key(self):
        return (self.max_columns, self.current_saft, self.slave_stop, self.extra_args, self.input_file.replace("./", ""))

    def start_message(self):
        message = """
==== (CURRENT TIME) ==== """ + str(today()) + """
Running """ + str(self.max_columns) + """ columns, saft set to """ + str(self.current_saft) + """, slavestop is """ + str(self.slave_stop) + """ on """ + str(self.output_file) + """, extra args is """ + str(self.extra_args) + """ :
Executing ,,""" + str(self.command()) + """'' ...
"""
        return message

    def set_output_file_from_input_and_saft(self, output_dir):
        if self.current_saft == 1:
            self.output_file = str(output_dir) + "/" + str(os.path.basename(self.input_file)) + "_log"
        else:
            self.output_file = str(output_dir) + "/" + str(os.path.basename(self.input_file)) + "_with_" + str(self.current_saft) + "_saft_log"

    """
    Returns the command executed by execute()
    """
    def command(self):
        command = str(self.exec_path)
        if len(self.input_file) > 0:
            command += " -C " + str(self.input_file)
        command += " " + str(self.extra_args)
        if self.slave_stop is not None:
            command +=" --slavestop " + str(self.slave_stop)
        command += " --max_columns " + str(self.max_columns) + " --saft " + str(self.current_saft) + " -o " + str(self.output_file)
        return command

    """
    Returns a readable version of the input_file.
    """
    def run_name(self):
        return os.path.basename(self.input_file.replace("_","-"))

    """
    Means the stats of the configs that share the same inputfiles/inputparameter
    """
    def mean(configlist):
        d = {}
        for config in configlist:
            d.setdefault(config.key(), []).append(config)

        ret = []
        for mean_me in d.values():
            new_stats = statistics()
            new_config = copy.deepcopy(mean_me[0])

            times = map(lambda x: x.stats.user_time, mean_me)
            new_stats.user_time = float(sum(times))/len(mean_me)

            times = map(lambda x: x.stats.system_time, mean_me)
            new_stats.system_time = float(sum(times))/len(mean_me)

            new_config.stats = new_stats
            ret.append(new_config)
        return ret

    """
    Executes the current run and fill stats.
    """
    def execute(self):
        command = self.command()
        ru_before = time_needed_by_spawned_processes()
        return_status = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, shell=True)

        ru = time_needed_by_spawned_processes()
        self.stats.user_time = ru[0] - ru_before[0]
        self.stats.system_time = ru[1] - ru_before[1]
        #TODO: add more infos?

        return return_status



"""
Writes a string into 2 different outputs
"""
class log:
    output = None
    def __init__(self, output):
        self.output = output

    def write(self, string):
        if (self.output is not None):
            self.output.write(string + '\n')
        sys.stdout.write(string + '\n')
"""
Removes empty lines from string.
"""
def strip_empty_lines(string):
    return str(string).replace('\n\n\n','')

"""
Executes one execution, is passed to launch() as callback and can be swapped with custom code.
"""
def single_run(timelog, config):
    timelog.write(config.start_message())
    status_code = config.execute()
    timelog.write("Exit code : " + str(status_code.returncode))
    timelog.write("StdOut : <<" + str(strip_empty_lines(status_code.stdout)) + ">>")
    timelog.write("StdErr : <<" + str(strip_empty_lines(status_code.stderr)) + ">>")
    timelog.write("")
    timelog.write("")

"""
Executes make.
"""
def make():
    print("executing make...")
    if "PANDORA" in os.environ:
        command = 'make -j3 PANDORA=1'
    else:
        command = 'make -j3'
    status = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True, shell=True)
    if status.returncode != 0:
        print(status.stdout)
        return False
    print("make succesful!")
    return True



"""
Launches the reconstruction for the files given in sys.argv. Extra_config_generator is a lambda that takes one config and returns a list of configs that tries to make this script more flexible (eg when testing multiple solvers).
"""
def launch(files=sys.argv[1:], extra_config_generator=lambda x: [x], config_implementation=config):
    if (len(files) == 0):
        print("Error: no input files!")
        return None

    current_commit = subprocess.run('echo -n "$(date --rfc-3339=seconds | tr \' \' -)-$(git rev-parse HEAD)"', stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True, shell=True).stdout

    folder = os.environ.get("FOLDER", ".")
    output_dir = "./" + str(folder) + "/" + str(current_commit)
    exec_path = "./" + str(folder) + "/" + str(current_commit) + "/EXEC"
    retarget_path = "./" + str(folder) + "/" + str(current_commit) + "/RETARGET"

    os.makedirs(output_dir, exist_ok=True)

    if not make():
        return False

    if "PANDORA" in os.environ:
        shutil.copy("./build8/opt", exec_path)
    else:
        shutil.copy("./build9/opt", exec_path)

    executed_configs = []

    current_dump = 0

    print("Using directory " + str(output_dir))
    with open("./" + str(output_dir) + "/time", 'a') as timelogstream:
        timelog = log(timelogstream)
        for input_file in files:
            safts = [ 1 ]
            if "SAFT" in os.environ:
                safts.append(os.environ["SAFT"])
            for current_saft in safts:
                c = config_implementation(exec_path, input_file, current_saft)
                c.set_output_file_from_input_and_saft(output_dir)

                for current_config in extra_config_generator(c):
                    single_run(timelog, current_config)
                    executed_configs.append(current_config)
                    #saves the executed_configs so it can be loaded and reworked later
                    with open("./" + str(output_dir) + "/IntermediatePythonData_"+ str(current_dump) +".pkl", 'wb') as output:
                        pickle.dump(executed_configs, output, pickle.HIGHEST_PROTOCOL)
                        current_dump = current_dump + 1

    #saves the executed_configs so it can be loaded and reworked later
    with open("./" + str(output_dir) + "/PythonData.pkl", 'wb') as output:
        pickle.dump(executed_configs, output, pickle.HIGHEST_PROTOCOL)
        current_dump = current_dump + 1

    return executed_configs

if __name__ == "__main__":
    launch()

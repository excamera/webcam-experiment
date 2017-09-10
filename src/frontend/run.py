import argparse
import datetime
import time
import os
import json
import sys
import random
from pykeyboard import PyKeyboard

parser = argparse.ArgumentParser()
parser.add_argument('output_dir', type=str, help='output_dir')
args = parser.parse_args()

wait = lambda : time.sleep(0.1)
k = PyKeyboard()

def switch_to_workspace(workspace):
    wait()
    k.press_key(k.super_l_key)
    k.tap_key(str(workspace))
    k.release_key(k.super_l_key)

def reset_workspace():
    wait()
    k.press_key(k.super_l_key)
    k.press_key(k.shift_l_key)
    for _ in range(2):
        k.tap_key('q')
        k.tap_key(k.return_key)
        time.sleep(.5)
        
    k.release_key(k.shift_l_key)

    k.tap_key(k.return_key)
    k.release_key(k.super_l_key)

def maximize_window():
    wait()
    k.press_key(k.super_l_key)
    k.tap_key('f')
    k.release_key(k.super_l_key)
    
def run_command(command):
    wait()
    for key in command:
        k.tap_key(key)
    wait()
        
    k.tap_key(k.return_key)

def run_experiment(runtime=120,
                   quantizer=48, delay=30,
                   before1='/dev/null', before2='/dev/null',
                   after1='/dev/null', after2='/dev/null'):

    os.system('killall my-camera')

    switch_to_workspace(1)
    reset_workspace()
    switch_to_workspace(2)
    reset_workspace()
    
    switch_to_workspace(1)
    run_command('cd /home/jemmons/projects/webcam-experiment/src/frontend')
    run_command('./run2.sh --before-file {} --after-file {} --quantizer {} --delay {}'.format( \
        before1, after1, str(quantizer), str(delay) \
    ))
    time.sleep(3)
    maximize_window()
    
    switch_to_workspace(2)
    run_command('cd /home/jemmons/projects/webcam-experiment/src/frontend')
    run_command('./run1.sh --before-file {} --after-file {} --quantizer {} --delay {}'.format( \
        before2, after2, str(quantizer), str(delay) \
    ))
    time.sleep(3)
    maximize_window()
    
    time.sleep(runtime)
    os.system('killall my-camera')
    time.sleep(2)

    switch_to_workspace(1)
    run_command('clear')
    run_command('echo -e "\\n\\n--------------------------------------------------------------------------------\\n\\n    Please record your quality of experience on the worksheet (1 to 5 score)\\n    The next video chat will begin in 30 seconds\\n\\n--------------------------------------------------------------------------------\\n\\n"')

    time.sleep(1)
    
    switch_to_workspace(2)
    run_command('clear')
    run_command('echo -e "\\n\\n--------------------------------------------------------------------------------\\n\\n    Please record your quality of experience on the worksheet (score 1 to 5) \\n   The next video chat will begin in 30 seconds\\n\\n--------------------------------------------------------------------------------\\n\\n"')
    time.sleep(5)
    os.system('killall my-camera')
    
settings = [
    {
        'quantizer' : 26,
        'delay' : 8
    },
    {
        'quantizer' : 33,
        'delay' : 8
    },
    {
        'quantizer' : 40,
        'delay' : 8
    },
    {
        'quantizer' : 26,
        'delay' : 15
    },
    {
        'quantizer' : 33,
        'delay' : 15
    },
    {
        'quantizer' : 40,
        'delay' : 15
    }
    ]

random.shuffle(settings)

settings = [
    {
        'quantizer' : 26,
        'delay' : 1
    }
] + settings

experiment_time = 30

# create the output dir for the results
if os.path.isdir(args.output_dir):
    print('{} already exists. '.format(args.output_dir) + \
          'Please remove it before running the experiment')
    sys.exit(0)
    
os.makedirs(args.output_dir)
    
# serialize all the settings for this experiment
with open(os.path.join(args.output_dir, 'settings.json'), 'w') as f:
    metadata = dict()
    metadata['datetime'] = str(datetime.datetime.now())
    metadata['experiment_time'] = experiment_time
    metadata['settings'] = settings
    f.write(json.dumps(metadata, indent=4, sort_keys=True))
        
for setting in settings:
    print(setting)

    results_dirname = 'q'+str(setting['quantizer']) + '-' + \
                      'd'+ str(setting['delay'])

    results_dir_path = os.path.join(args.output_dir, results_dirname)
    os.makedirs(results_dir_path)

    run_experiment(runtime=experiment_time, \
                   quantizer=setting['quantizer'], delay=setting['delay'],
                   before1=os.path.join(results_dir_path,'before1.y4m'),
                   before2=os.path.join(results_dir_path,'before2.y4m'),
                   after1=os.path.join(results_dir_path,'after1.y4m'),
                   after2=os.path.join(results_dir_path,'after2.y4m')
                   )

    

# Inspired by https://github.com/liblava/liblava/blob/flow/ext/update.py

import os, json, requests

dir_path = os.path.dirname(os.path.realpath(__file__))

with open(dir_path + "/CPM.cmake", "w+") as cpm_file:
    response = requests.get("https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/get_cpm.cmake")
    response.raise_for_status()
    cpm_file.write(response.text)

with open(dir_path + "\dependencies.json") as f:
    deps = json.load(f)
    with open('version.cmake', 'w+') as version_file:
        for dep in deps:
            name = dep['name']
            github = dep['github']
            branch = dep['branch']
            response = requests.get(f'https://api.github.com/repos/{github}/branches/{branch}')
            response.raise_for_status()
            info = json.loads(response.text)

            if 'commit' not in info:
                raise ValueError(info)

            if 'sha' not in info['commit']:
                raise ValueError(info)

            tag = info['commit']['sha']

            version_file.write('\nset(' + name + '_GITHUB ' + github + ')\n')
            version_file.write('set(' + name + '_TAG ' + tag + ')\n')
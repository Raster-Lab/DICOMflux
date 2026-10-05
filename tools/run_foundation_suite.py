"""Run bounded DF-A002 new-code assurance on the recorded macOS host.

No downloads, global installs, historical rewrites or consumer-platform claims.
Use a fresh run id and private evidence directory. Every command is retained.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import struct
import subprocess
import sys
import time


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('cmake', 'ninja', 'python', 'fuzzer-runtime', 'evidence-dir'):
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--run-id', required=True)
    parser.add_argument('--configs', default='host,address,undefined,thread,fuzz')
    args = parser.parse_args()
    require(platform.system() == 'Darwin' and platform.machine() == 'arm64', 'Runner is scoped to the observed macOS arm64 host')
    require(re.fullmatch(r'[a-zA-Z0-9_-]+', args.run_id), 'Invalid run id')
    configs = args.configs.split(',')
    require(len(configs) == len(set(configs)) and set(configs) <= {'host','address','undefined','thread','fuzz'}, 'Invalid configurations')
    require(bool(configs), 'Empty suite')
    repo = Path(__file__).resolve().parents[1]
    evidence = args.evidence_dir.resolve()
    require(repo not in evidence.parents and evidence != repo, 'Raw evidence must be outside the checkout')
    require(not evidence.exists(), 'Evidence directory exists; preserve it and choose a fresh one')
    run_id = args.run_id
    dirs = [repo / 'build' / (run_id + '-' + c) for c in ['host','address','undefined','thread','fuzz','consumers','consumer-src','corpus']]
    dirs += [repo / 'install' / run_id, repo / 'install' / (run_id + '-retained-original'), repo / 'stage' / run_id]
    require(not any(p.exists() for p in dirs), 'Build outputs exist; choose a new run id')
    evidence.mkdir(parents=True)
    cmake, ninja, python, fuzzer = (str(p.resolve()) for p in (args.cmake,args.ninja,args.python,args.fuzzer_runtime))
    ctest = str(Path(cmake).with_name('ctest'))
    tools = [cmake, ninja, python, ctest, fuzzer, '/usr/bin/clang', '/usr/bin/clang++']
    for tool in tools:
        require(Path(tool).is_file(), 'Missing selected tool: ' + tool)
    paths = [repo/'CMakeLists.txt', repo/'CMakePresets.json', repo/'AGENTS.md', repo/'README.md']
    for base in ['src','include','cmake','tests/unit','tests/foundation','tests/foundation_consumer',
                 'tests/regression','tests/fixtures','fuzz','tools','bindings/python_probe','docs/architecture',
                 'docs/profiles','docs/evidence/authorisations']:
        paths += [p for p in (repo/base).rglob('*') if p.is_file() and '__pycache__' not in p.parts]
    paths.append(repo/'docs/evidence/effective-authority.json')
    source = {str(p.relative_to(repo)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(set(paths))}
    for name in source:
        target = evidence/'inputs'/name; target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(repo/name,target)
    (evidence/'input-manifest.json').write_text(json.dumps(source,indent=2)+'\n')
    def git(*arguments):
        return subprocess.check_output(['git',*arguments],cwd=repo,text=True).strip()
    environment = {'captured_before_execution':datetime.now(timezone.utc).isoformat(), 'repo':str(repo),
                   'head':git('rev-parse','HEAD'), 'head_tree':git('rev-parse','HEAD^{tree}'),
                   'branch':git('branch','--show-current'), 'state':git('status','--porcelain=v1','--untracked-files=all'),
                   'python':sys.version,'runner_optimize':sys.flags.optimize,'platform':platform.platform(),'machine':platform.machine(),
                   'tool_sha256':{p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in tools},
                   'configurations':configs, 'sanitizer_environment':{'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1',
                      'ASAN_OPTIONS':'abort_on_error=1:allocator_may_return_null=1','TSAN_OPTIONS':'halt_on_error=1'},
                   'scope':'Auxiliary host only; not Linux, Android/JNI or Pi qualification'}
    (evidence/'identity-before.json').write_text(json.dumps(environment,indent=2)+'\n')
    source_digest=hashlib.sha256(json.dumps(source,sort_keys=True).encode()).hexdigest()
    env=os.environ.copy();env.pop('PYTHONOPTIMIZE',None);env.update(environment['sanitizer_environment'])
    records=[]
    def run(name, command, kind='foundation_test', expected=0, input_text=None):
        command=list(map(str,command));start=time.monotonic();when=datetime.now(timezone.utc).isoformat()
        result=subprocess.run(command,cwd=repo,env=env,text=True,input=input_text,capture_output=True)
        outcome=('passed' if expected==0 else 'expected_rejection') if result.returncode==expected else 'failed'
        record={'id':name,'kind':kind,'command':command,'cwd':str(repo),'started_at':when,
                'elapsed_seconds':time.monotonic()-start,'exit_code':result.returncode,'expected_exit_code':expected,
                'outcome':outcome,'stdout':result.stdout,'stderr':result.stderr,'input_text':input_text,
                'source_digest':source_digest,'scope':'DF-A002 foundation-only host evidence'}
        (evidence/(name+'.json')).write_text(json.dumps(record,indent=2)+'\n');records.append(record)
        print(name,outcome,'exit='+str(result.returncode),flush=True)
        if result.returncode!=expected:print((result.stdout+result.stderr)[-4000:],flush=True)
        return result.returncode==expected
    for name,command in [('cmake-version',[cmake,'--version']),('ninja-version',[ninja,'--version']),
                         ('compiler-version',['/usr/bin/clang++','--version']),('python-version',[python,'--version']),
                         ('os-version',['/usr/bin/sw_vers']),('sdk-version',['/usr/bin/xcrun','--show-sdk-version']),
                         ('sdk-path',['/usr/bin/xcrun','--show-sdk-path']),('page-size',['/usr/bin/getconf','PAGESIZE'])]:
        run(name,command,'environment')
    run('stdlib-macros',['/usr/bin/clang++','-std=c++20','-dM','-E','-x','c++','-'],'environment',input_text='#include <version>\n')
    for config in configs:
        build=repo/'build'/(run_id+'-'+config)
        options=['-DBUILD_TESTING=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON']
        if config=='host':options+=['-DCMAKE_INSTALL_PREFIX='+str(repo/'install'/run_id)]
        elif config=='fuzz':options+=['-DBUILD_TESTING=OFF','-DDICOMFLUX_SANITIZER=address','-DDICOMFLUX_BUILD_FOUNDATION_FUZZER=ON','-DDICOMFLUX_FUZZER_RUNTIME='+fuzzer]
        else:options+=['-DDICOMFLUX_SANITIZER='+config]
        if not run(config+'-configure',[cmake,'-S',repo,'-B',build,'-G','Ninja','-DCMAKE_MAKE_PROGRAM='+ninja,
              '-DCMAKE_BUILD_TYPE=Debug','-DCMAKE_C_COMPILER=/usr/bin/clang','-DCMAKE_CXX_COMPILER=/usr/bin/clang++',*options],'build'):continue
        target=[] if config=='host' else ['--target','df1_foundation_fuzz' if config=='fuzz' else 'df1_foundation_tests']
        if not run(config+'-build',[cmake,'--build',build,'--parallel','2',*target],'build'):continue
        if config!='fuzz':
            selection=[] if config=='host' else ['-R','^df1_foundation$']
            run(config+'-tests',[ctest,'--test-dir',build,'--verbose','--output-on-failure',*selection])
        if config=='host':
            if not run('install',[cmake,'--install',build],'install'):continue
            stage=repo/'stage'/run_id;shutil.copytree(repo/'install'/run_id,stage)
            (repo/'install'/run_id).rename(repo/'install'/(run_id+'-retained-original'))
            source_dir=repo/'build'/(run_id+'-consumer-src');shutil.copytree(repo/'tests/foundation_consumer',source_dir)
            consumers=repo/'build'/(run_id+'-consumers')
            if run('consumer-configure',[cmake,'-S',source_dir,'-B',consumers,'-G','Ninja','-DCMAKE_MAKE_PROGRAM='+ninja,
                   '-DCMAKE_C_COMPILER=/usr/bin/clang','-DCMAKE_CXX_COMPILER=/usr/bin/clang++',
                   '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','-DCMAKE_PREFIX_PATH='+str(stage),'-DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF'],'install'):
                if run('consumer-build',[cmake,'--build',consumers,'--parallel','2'],'install'):
                    run('installed-consumers',[ctest,'--test-dir',consumers,'--verbose'],'installed_consumer')
                    compile_commands=(consumers/'compile_commands.json').read_text()
                    require(str(repo/'include') not in compile_commands and str(repo/'src') not in compile_commands, 'Source include leakage')
                    (evidence/'consumer-compile-commands.json').write_text(compile_commands)
            library=stage/'lib/libdicomflux_foundation.dylib';capabilities=stage/'share/dicomflux/foundation-capabilities.json'
            bad=json.loads(capabilities.read_text());bad['functions'].append('dicomflux_cancel_request')
            bad_path=evidence/'invalid-capabilities.json';bad_path.write_text(json.dumps(bad,indent=2)+'\n')
            bad_source=json.loads(capabilities.read_text());bad_source['source_sha256']={}
            bad_source_path=evidence/'invalid-source-capabilities.json';bad_source_path.write_text(json.dumps(bad_source,indent=2)+'\n')
            (evidence/'negative-input-before.json').write_text(json.dumps({'files':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [bad_path,bad_source_path]},'captured_at':datetime.now(timezone.utc).isoformat()},indent=2)+'\n')
            regression_root=repo/'build/df0-regression-inputs'
            previous_regression_dirs=set(regression_root.glob('*'))
            for flags,label in [([], 'normal'),(['-O'],'optimized')]:
                run('boundary-'+label,[python,*flags,repo/'tools/check_foundation_boundary.py','--library',library,'--capabilities',capabilities],'binary_and_scope')
                run('boundary-negative-'+label,[python,*flags,repo/'tools/check_foundation_boundary.py','--library',library,'--capabilities',bad_path],'expected_negative',expected=1)
                run('boundary-missing-source-'+label,[python,*flags,repo/'tools/check_foundation_boundary.py','--library',library,'--capabilities',bad_source_path],'expected_negative',expected=1)
                run('df0-regressions-'+label,[python,*flags,repo/'tests/regression/test_df0_corrections.py','--library',stage/'lib/libdicomflux_probe.dylib'],'retained_regression')
            run('foundation-exports',['/usr/bin/nm','-gU',library],'binary_inspection')
            run('foundation-dependencies',['/usr/bin/otool','-L',library],'binary_inspection')
            run('probe-boundary',[python,repo/'tools/check_probe_binary.py',stage/'lib/libdicomflux_probe.dylib'],'separate_prototype_inspection')
            shutil.copy2(capabilities,evidence/'installed-capabilities.json')
            retained=evidence/'retained-regression-inputs';retained.mkdir()
            for directory in sorted(set(regression_root.glob('*'))-previous_regression_dirs):
                shutil.copytree(directory,retained/directory.name)
        if config=='fuzz':
            corpus=repo/'build'/(run_id+'-corpus');corpus.mkdir()
            seeds=[(0,0,0),(1,4096,0),(2**64-1,4096,0),(2**64-1,1,3),(2**32,4096,6),(15,4096,0)]
            for index,(a,b,selector) in enumerate(seeds):
                (corpus/str(index)).write_bytes(struct.pack('<QQBBB',a,b,selector,index%4,255))
            def snapshot(label):
                target=evidence/label;shutil.copytree(corpus,target)
                mapping={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(target.iterdir()) if p.is_file()}
                (evidence/(label+'.json')).write_text(json.dumps({'captured_at':datetime.now(timezone.utc).isoformat(),'files':mapping},indent=2)+'\n')
            snapshot('corpus-before-short')
            run('fuzz-short',[build/'df1_foundation_fuzz',corpus,'-seed=20261005','-max_total_time=5','-max_len=96','-print_final_stats=1','-artifact_prefix='+str(evidence)+'/'],'coverage_guided_fuzz')
            snapshot('corpus-before-longer')
            run('fuzz-longer',[build/'df1_foundation_fuzz',corpus,'-seed=20261006','-max_total_time=15','-max_len=96','-print_final_stats=1','-artifact_prefix='+str(evidence)+'/'],'coverage_guided_fuzz')
            snapshot('corpus-after-longer')
        if (build/'compile_commands.json').exists():shutil.copy2(build/'compile_commands.json',evidence/(config+'-compile-commands.json'))
    (evidence/'suite.json').write_text(json.dumps([{k:v for k,v in r.items() if k not in ['stdout','stderr']} for r in records],indent=2)+'\n')
    return int(any(r['outcome']=='failed' for r in records))

if __name__ == '__main__':
    raise SystemExit(main())

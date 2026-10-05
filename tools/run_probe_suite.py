#!/usr/bin/env python3
"""Run DF-0 probes with exact caller-selected tools; never fetch dependencies.

Build outputs stay below the source checkout. Raw evidence goes to the separately
specified private directory. A failed command is recorded and the suite exits
nonzero while continuing independent sanitizer configurations.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import platform
import re
import time

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--cmake',type=Path,required=True)
    parser.add_argument('--ninja',type=Path,required=True)
    parser.add_argument('--python',type=Path,required=True)
    parser.add_argument('--evidence-dir',type=Path,required=True)
    parser.add_argument('--configs',default='host,address,undefined,thread,fuzz')
    parser.add_argument('--fuzzer-runtime',type=Path)
    parser.add_argument('--run-id', default='df0-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%f'))
    args=parser.parse_args()
    if not re.fullmatch(r'[a-zA-Z0-9_-]+',args.run_id):
        parser.error('run-id must be a simple directory name')
    repo=Path(__file__).resolve().parents[1]
    run_id=args.run_id
    output_dirs=[repo/'build'/(run_id+'-'+c) for c in ['host','address','undefined','thread','fuzz','consumer-src','consumers','fuzz-corpus']]
    output_dirs += [repo/'install'/run_id,repo/'stage'/run_id]
    if any(p.exists() for p in output_dirs):
        parser.error('run-id already has outputs; choose a fresh run-id to preserve them')
    evidence=args.evidence_dir.resolve()
    if evidence.exists() and any(evidence.iterdir()):
        parser.error('Evidence directory must be empty; retain previous runs separately.')
    configs=args.configs.split(',')
    if not configs or any(c not in ['host','address','undefined','thread','fuzz'] for c in configs):
        parser.error('Unknown or empty configuration; no successful empty suite.')
    evidence.mkdir(parents=True,exist_ok=True)
    cmake=str(args.cmake.resolve()); ninja=str(args.ninja.resolve())
    python=str(args.python.resolve()); ctest=str(args.cmake.resolve().with_name('ctest'))
    results=[]
    source_files=[]
    for base in ['src','include','tests/unit','tests/installed_consumer','tests/regression',
                 'tests/fixtures','fuzz','cmake','bindings/python_probe','tools','docs/profiles','docs/architecture']:
        source_files.extend(p for p in (repo/base).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    source_files += [repo/'CMakeLists.txt',repo/'CMakePresets.json']
    source_identity={str(p.relative_to(repo)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(source_files)}
    (evidence/'build-source-files.json').write_text(json.dumps(source_identity,indent=2)+'\n')
    for name in source_identity:
        destination=evidence/'inputs'/name
        destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(repo/name,destination)
    (evidence/'environment.json').write_text(json.dumps({
        'python':sys.version,'runner_optimize':sys.flags.optimize,'platform':platform.platform(),
        'machine':platform.machine(),'PYTHONOPTIMIZE':os.environ.get('PYTHONOPTIMIZE'),
        'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1',
        'tools':{p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in
                 [cmake,ninja,python,'/usr/bin/clang','/usr/bin/clang++']}},indent=2)+'\n')
    source_digest=hashlib.sha256(json.dumps(source_identity,sort_keys=True).encode()).hexdigest()
    def run(name,command,kind='compile_link_probe',cwd=repo,env=None):
        start=time.monotonic(); when=datetime.now(timezone.utc).isoformat()
        process=subprocess.run([str(x) for x in command],cwd=cwd,env=env,text=True,capture_output=True)
        record={'schema_version':'1','id':name,'kind':kind,'command':[str(x) for x in command],
                'cwd':str(cwd),'started_at':when,'elapsed_seconds':time.monotonic()-start,
                'exit_code':process.returncode,'outcome':'passed' if process.returncode==0 else 'failed',
                'build_source_digest':source_digest,
                'stdout':process.stdout,'stderr':process.stderr,'scope':'DF-0 prototype only'}
        (evidence/(name+'.json')).write_text(json.dumps(record,indent=2)+'\n')
        (evidence/(name+'.log')).write_text(process.stdout+process.stderr)
        results.append({k:v for k,v in record.items() if k not in ['stdout','stderr']})
        print(name,record['outcome'],f"{record['elapsed_seconds']:.2f}s",flush=True)
        if process.returncode: print((process.stdout+process.stderr)[-3500:],flush=True)
        return process.returncode==0
    env=os.environ.copy()
    env['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1'
    for name,cmd in [('cmake-version',[cmake,'--version']),('ninja-version',[ninja,'--version']),
                     ('python-version',[python,'--version']),('compiler-version',['/usr/bin/clang++','--version'])]:
        if not run(name,cmd):return 1
    for config in configs:
        build=repo/'build'/(run_id+'-'+config)
        extra=[]
        if config=='host':extra=['-DCMAKE_INSTALL_PREFIX='+str(repo/'install'/run_id)]
        elif config=='fuzz':
            extra=['-DDICOMFLUX_BUILD_FUZZER=ON','-DBUILD_TESTING=OFF','-DDICOMFLUX_SANITIZER=address']
            if args.fuzzer_runtime:extra+=['-DDICOMFLUX_FUZZER_RUNTIME='+str(args.fuzzer_runtime.resolve())]
        else:extra=['-DDICOMFLUX_SANITIZER='+config]
        configured=run(config+'-configure',[cmake,'-S',repo,'-B',build,'-G','Ninja',
            '-DCMAKE_MAKE_PROGRAM='+ninja,'-DCMAKE_BUILD_TYPE=Debug',
            '-DCMAKE_C_COMPILER=/usr/bin/clang','-DCMAKE_CXX_COMPILER=/usr/bin/clang++',*extra])
        if not configured:continue
        if not run(config+'-build',[cmake,'--build',build,'--parallel','2']):continue
        if config!='fuzz':
            run(config+'-unit',[ctest,'--test-dir',build,'--verbose','--output-on-failure'],'foundation_test',env=env)
        if config=='host':
            if not run('host-install',[cmake,'--install',build]):continue
            stage=repo/'stage'/run_id
            shutil.copytree(repo/'install'/run_id,stage)
            source=repo/'build'/(run_id+'-consumer-src')
            shutil.copytree(repo/'tests/installed_consumer',source)
            consumer=repo/'build'/(run_id+'-consumers')
            if run('consumer-configure',[cmake,'-S',source,'-B',consumer,'-G','Ninja',
                    '-DCMAKE_MAKE_PROGRAM='+ninja,'-DCMAKE_PREFIX_PATH='+str(stage),
                    '-DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF']):
                if run('consumer-build',[cmake,'--build',consumer,'--parallel','2']):
                    run('consumer-run',[ctest,'--test-dir',consumer,'--verbose'],'installed_consumer_test')
            suffix='dylib' if os.uname().sysname=='Darwin' else 'so'
            library=stage/'lib'/('libdicomflux_probe.'+suffix)
            run('python-probe',[python,repo/'bindings/python_probe/probe.py',library,
                '--budgets',repo/'docs/architecture/qualification-budgets.json'],'binding_probe')
            run('python-probe-optimized',[python,'-O',repo/'bindings/python_probe/probe.py',library,
                '--budgets',repo/'docs/architecture/qualification-budgets.json'],'binding_probe')
            run('correction-regressions',[python,repo/'tests/regression/test_df0_corrections.py',
                '--library',library],'foundation_test')
            run('binary-policy',[python,repo/'tools/check_probe_binary.py',library],'binary_inspection')
            run('owned-fixture-generate',[python,repo/'tools/generate_synthetic_pixels.py'],'fixture_generation')
            run('profile-spec-check',[python,repo/'tools/validate_profile_inventory.py'],'document_check')
            run('resource-observation',[python,repo/'tools/measure_probe_resources.py',build/'df0_unit'],'resource_measurement')
            if suffix=='dylib':
                run('host-exports',['nm','-gU',library],'binary_inspection')
                run('host-dependencies',['otool','-L',library],'binary_inspection')
            else:
                run('host-exports',['nm','-D','--defined-only',library],'binary_inspection')
                run('host-dependencies',['readelf','-d',library],'binary_inspection')
        if config=='fuzz':
            corpus=repo/'build'/(run_id+'-fuzz-corpus'); corpus.mkdir()
            for index,values in enumerate([(0,0,0,0),(16,16,768,0),(2**64-1,2,0,1),
                                          (2**32-2,1,0,0),(7,7,8,4),(1,1,3,8)]):
                (corpus/str(index)).write_bytes(struct.pack('<QQQQ',*values))
            def save_corpus(label):
                target=evidence/label
                shutil.copytree(corpus,target)
                entries={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(target.iterdir()) if p.is_file()}
                (evidence/(label+'.json')).write_text(json.dumps(entries,indent=2)+'\n')
            save_corpus('corpus-before-short')
            run('fuzz-short',[build/'df0_fuzz',corpus,'-seed=20261004','-max_total_time=10',
                '-max_len=64','-print_final_stats=1'],'coverage_guided_fuzz',env=env)
            save_corpus('corpus-before-longer')
            run('fuzz-longer',[build/'df0_fuzz',corpus,'-seed=20261005','-max_total_time=30',
                '-max_len=64','-print_final_stats=1'],'coverage_guided_fuzz',env=env)
            save_corpus('corpus-after-longer')
            corpus_manifest=[{'name':p.name,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
                              'bytes':p.stat().st_size} for p in sorted(corpus.iterdir()) if p.is_file()]
            (evidence/'fuzz-corpus.json').write_text(json.dumps(corpus_manifest,indent=2)+'\n')
    (evidence/'suite.json').write_text(json.dumps(results,indent=2)+'\n')
    return int(any(r['exit_code'] for r in results))

if __name__=='__main__':raise SystemExit(main())

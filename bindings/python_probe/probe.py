"""DF-0 ctypes load/call/close witness. No DICOM SDK or file writer."""
import argparse
import ctypes as C
import gc
import json
import platform
import sys
from pathlib import Path

class Options(C.Structure):
    _fields_ = [(n,C.c_uint32) for n in ('struct_size','abi_major','flags','reserved')] + [
        (n,C.c_uint64) for n in ('metadata','attributes','bulk','tracked','chunk')] + [
        (n,C.c_void_p) for n in ('allocator_user','allocate','deallocate')]
class Measurement(C.Structure):
    _fields_ = [(n,C.c_uint64) for n in ('logical','encoded','padding','tracked','peak')]
class Abi(C.Structure):
    _fields_ = [(n,C.c_uint32) for n in ('size','major','minor','pointer_bits','options_size','alignment','libcxx','reserved')]

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('library',type=Path)
    parser.add_argument('--budgets',type=Path,required=True)
    args=parser.parse_args()
    settings=json.loads(args.budgets.read_text())
    library=C.CDLL(str(args.library.resolve()))
    library.dicomflux_probe_query_abi.argtypes=[C.c_uint32,C.c_size_t,C.POINTER(Abi)]
    library.dicomflux_probe_query_abi.restype=C.c_int32
    library.dicomflux_probe_create.argtypes=[C.POINTER(Options),C.POINTER(C.c_void_p)]
    library.dicomflux_probe_create.restype=C.c_int32
    library.dicomflux_probe_measure.argtypes=[C.c_void_p]+[C.c_uint64]*5+[C.POINTER(Measurement)]
    library.dicomflux_probe_measure.restype=C.c_int32
    library.dicomflux_probe_release.argtypes=[C.c_void_p]
    library.dicomflux_probe_release.restype=None
    abi=Abi()
    assert library.dicomflux_probe_query_abi(0,C.sizeof(abi),C.byref(abi))==0
    assert abi.options_size==C.sizeof(Options) and abi.alignment==C.alignment(Options)
    options=Options(C.sizeof(Options),0,0,0,settings['metadata_bytes'],settings['attributes'],
                    settings['declared_bulk_bytes'],settings['library_tracked_bytes'],settings['transfer_chunk_bytes'])
    class Owner:
        def __init__(self):
            self.pointer=C.c_void_p()
            assert library.dicomflux_probe_create(C.byref(options),C.byref(self.pointer))==0
        def close(self):
            if self.pointer.value:
                library.dicomflux_probe_release(self.pointer)
                self.pointer=C.c_void_p()
        def __enter__(self):return self
        def __exit__(self,*_):self.close()
    with Owner() as owner:
        gc.collect()
        m=Measurement()
        assert library.dicomflux_probe_measure(owner.pointer,16,16,3,100,20,C.byref(m))==0
        assert m.logical==768 and m.padding==0
        assert library.dicomflux_probe_measure(owner.pointer,1,1,3,0,0,C.byref(m))==0
        assert (m.logical,m.encoded,m.padding)==(3,4,1)
        assert library.dicomflux_probe_measure(owner.pointer,2**64-1,2,1,0,0,C.byref(m))==4
        assert m.logical==0
    owner.close()  # Wrapper-level idempotency; never pass a stale native handle.
    options.flags=1
    output=C.c_void_p()
    assert library.dicomflux_probe_create(C.byref(options),C.byref(output))==2 and not output.value
    print(json.dumps({'kind':'foundation_probe','python':sys.version,'machine':platform.machine(),
        'abi_options_size':abi.options_size,'abi_options_alignment':abi.alignment,
        'load_call_gc_explicit_close_double_wrapper_close_invalid_options':'passed',
        'engine_writer_tests':'not_run; no engine writer implemented'}))

if __name__=='__main__':main()

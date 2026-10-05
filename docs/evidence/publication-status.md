# Provisional DF-0 review checkpoint

This checkpoint preserves the eligible existing DF-0 foundation work before the
bounded corrections. It is a draft for review, not a release or DF-G0 acceptance.
There is no DICOM writer/parser. The following findings remain open at this commit:

| Finding | Required action |
| --- | --- |
| DF0-REV-001 | Separate required element presence, legal empty Type 2/2C values and fixed-fixture equality. |
| DF0-REV-002 | Specify proposed C API state/query and error value mappings. |
| DF0-REV-003 | Prevent Python optimization from skipping essential calls and acceptance checks. |
| DF0-REV-004 | Obtain Linux, Android/JNI/page-size and the 12 requested Pi board/OS witnesses. |
| DF0-REV-005 | Reconcile the DCMTK digest and qualify external oracle/standards coverage. |
| DF0-REV-006 | Retain unknown historical input/corpus identities; snapshot future inputs before runs. |
| DF0-REV-007 | Correct expanded-table/macro count wording. |

Earlier reports/records describe historical executions, including failures. A
passing historical document checker did not detect the design contradictions or
the optimized-Python false pass. Later corrections must not relabel those results.
Of 60 historical command records, 28 carry the retained final 16-file build-source
digest; 32 do not. The initial fuzz corpus was not separately frozen. Current
source equality cannot retroactively establish missing input identities.

Publication includes original project code, owned raw synthetic pixels, original
fixture decisions and narrow factual DICOM identifiers with links, plus sanitized
technical results. It includes no standard page captures, extracted normative
prose, toolkit archives/dictionaries, private control/approval records or reports,
credentials, patient records or personal filesystem paths. The MIT licence covers
the owned project material, not the referenced standard or external tools.

Original source and raw evidence are retained privately. Public command records
use documented path aliases. Their raw logs, exact path map, historical corpus,
and standards captures are not publicly supplied; exact historical replay is
therefore limited. Builds and current fixture checks use public project inputs;
tool installation and independent normative review require the linked external
sources. This branch is the public delivery; private archives are not PR assets.

Only DF-W00 through DF-W04 are in scope. DF-G0 requires human disposition and
DF-1 requires separate authority. The PR stays draft while material gaps remain.

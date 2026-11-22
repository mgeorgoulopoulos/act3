import hashlib
from pathlib import Path
import stat
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import release
from release_plan import plan

class PackagingTests(unittest.TestCase):
    def test_original_checksum_is_mandatory(self):
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/'bad.zip';path.write_bytes(b'not the original')
            with self.assertRaises(ValueError):release.verify_original(path)

    def test_rejects_path_traversal(self):
        with tempfile.TemporaryDirectory() as temp:
            temp=Path(temp);archive=temp/'bad.zip'
            with zipfile.ZipFile(archive,'w') as z:z.writestr('../escape',b'bad')
            with patch.object(release,'verify_original'):
                with self.assertRaises(ValueError):release.extract_original(archive,temp/'out')
            self.assertFalse((temp/'escape').exists())

    def test_combined_zip_preserves_original_and_linux_execute_bit(self):
        with tempfile.TemporaryDirectory() as temp:
            temp=Path(temp);archive=temp/'original.zip'
            with zipfile.ZipFile(archive,'w') as z:
                z.writestr('ACT3/Act3.exe',b'original-executable')
                z.writestr('ACT3/data/textures/dna.PNG',b'neon')
                z.writestr('ACT3/data/Thumbs.db',b'cache')
            for platform in ['windows','linux']:
                base=temp/platform;(base/'data/textures').mkdir(parents=True)
                (base/'data/textures/dna.png').write_bytes(b'neon')
                (base/'asset-manifest.txt').write_text('4|data/textures/dna.png\n')
            (temp/'windows/act3.exe').write_bytes(b'new-windows-executable')
            (temp/'linux/act3').write_bytes(b'\x7fELFnew-linux-executable')
            (temp/'linux/linux/lib').mkdir(parents=True)
            (temp/'linux/linux/lib/libpng16.so.16').write_bytes(b'helper')
            output=temp/'release/ACT3.zip'
            with patch.object(release,'verify_original'):
                release.assemble(archive,temp/'windows',temp/'linux',output)
            with zipfile.ZipFile(output) as z:
                self.assertIsNone(z.testzip())
                self.assertEqual(z.read('ACT3/ACT3-2004.exe'),b'original-executable')
                self.assertEqual(z.read('ACT3/ACT3.exe'),b'new-windows-executable')
                self.assertTrue(z.getinfo('ACT3/ACT3-linux-x86_64').external_attr>>16 & stat.S_IXUSR)
                self.assertNotIn('ACT3/data/Thumbs.db',z.namelist())
                self.assertEqual(len(z.namelist()),len({n.casefold() for n in z.namelist()}))
            self.assertTrue(output.with_suffix('.zip.sha256').read_text().startswith(hashlib.sha256(output.read_bytes()).hexdigest()))

    def test_bad_manifest_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            base=Path(temp);(base/'asset-manifest.txt').write_text('123|missing.png\n')
            with self.assertRaises(ValueError):release.check_manifest(base)

class ReleaseTriggerTests(unittest.TestCase):
    def test_initial_version_publishes(self):self.assertTrue(plan('0.1.0','push','refs/heads/main'))
    def test_unreleased_version_retries(self):self.assertTrue(plan('0.1.0','push','refs/heads/main','0.1.0',False))
    def test_ordinary_commit_does_not_republish(self):self.assertFalse(plan('0.1.0','push','refs/heads/main','0.1.0',True))
    def test_next_version_publishes(self):self.assertTrue(plan('0.2.0','push','refs/heads/main','0.1.0',False))
    def test_version_cannot_go_backwards(self):
        with self.assertRaises(ValueError):plan('0.1.0','push','refs/heads/main','0.2.0',False)
    def test_pull_request_never_publishes(self):self.assertFalse(plan('0.2.0','pull_request','refs/pull/1/merge','0.1.0'))
    def test_manual_publish_requires_main(self):
        with self.assertRaises(ValueError):plan('0.1.0','workflow_dispatch','refs/heads/dev',manual=True)

if __name__=='__main__':unittest.main()

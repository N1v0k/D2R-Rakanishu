import java.nio.file.Files

def root = Files.createTempDirectory('signing-test').toFile()
try {
    def ext = new Expando()
    new GroovyShell(new Binding([ext: ext])).evaluate(new File(args[0]))
    def resolve = ext.dmResolveSigning
    def repo = new File(root, 'repo'); repo.mkdirs()
    def external = new File(root, 'private'); external.mkdirs()
    def key = new File(external, 'existing.keystore'); key.text = 'fixture'
    def props = new File(external, 'signing.properties')
    assert resolve(repo, key, [:], props.path).fallback
    def reject = { Closure call ->
        boolean rejected = false
        try { call() } catch (IllegalArgumentException expected) { rejected = true }
        assert rejected
    }
    reject { resolve(repo, new File(root, 'absent'), [:], props.path) }
    assert !new File(root, 'absent').exists()
    props.text = 'storeFile=existing.keystore\nkeyAlias=release\nstorePassword=store\nkeyPassword=key\nstoreType=PKCS12\n'
    def result = resolve(repo, key, [:], props.path)
    assert !result.fallback && result.storeFile == key.canonicalFile
    assert result.storePassword == 'store' && result.keyPassword == 'key' && result.keyAlias == 'release'
    assert result.storeType == 'PKCS12'
    assert resolve(repo, key, [DM_ANDROID_KEY_ALIAS: 'env'], props.path).keyAlias == 'env'
    props.delete()
    result = resolve(repo, key, [DM_ANDROID_KEYSTORE: key.path, DM_ANDROID_KEY_ALIAS: 'env',
        DM_ANDROID_STORE_PASSWORD: 'env-store', DM_ANDROID_KEY_PASSWORD: 'env-key'], props.path)
    assert result.storePassword == 'env-store' && result.keyPassword == 'env-key'
    reject { resolve(repo, key, [DM_ANDROID_KEY_ALIAS: 'partial'], props.path) }
    props.text = 'keyAlias=partial\n'
    reject { resolve(repo, key, [:], props.path) }
    def internal = new File(repo, 'signing.properties'); internal.text = props.text
    reject { resolve(repo, key, [:], internal.path) }
    props.text = 'storeFile=missing\nkeyAlias=x\nstorePassword=x\nkeyPassword=x\n'
    reject { resolve(repo, key, [:], props.path) }
    new File(external, '.git').mkdir()
    new File(external, '.git/HEAD').text = 'ref: refs/heads/main'
    reject { resolve(repo, key, [:], props.path) }
    new File(external, '.git').deleteDir()
    def home = new File(root, 'home'); new File(home, '.git').mkdirs()
    new File(home, '.git/HEAD').text = 'ref: refs/heads/main'
    def absent = new File(home, '.config/d2r-release/signing.properties')
    assert resolve(repo, key, [:], absent.path).fallback
    assert !resolve(repo, key, [DM_ANDROID_KEYSTORE: key.path, DM_ANDROID_KEY_ALIAS: 'env',
        DM_ANDROID_STORE_PASSWORD: 'store', DM_ANDROID_KEY_PASSWORD: 'key'], absent.path).fallback
    absent.parentFile.mkdirs(); absent.text = 'keyAlias=x\n'
    reject { resolve(repo, key, [:], absent.path) }
    def insideKey = new File(home, 'release.keystore'); insideKey.text = 'fixture'
    absent.delete()
    reject { resolve(repo, key, [DM_ANDROID_KEYSTORE: insideKey.path, DM_ANDROID_KEY_ALIAS: 'env',
        DM_ANDROID_STORE_PASSWORD: 'store', DM_ANDROID_KEY_PASSWORD: 'key'], absent.path) }
    reject { ext.dmCheckSigning(true, true, false) }
    ext.dmCheckSigning(true, true, true)
    ext.dmCheckSigning(true, false, false)
    ext.dmCheckSigning(false, true, false)
    println 'SigningTest: OK'
} finally { root.deleteDir() }

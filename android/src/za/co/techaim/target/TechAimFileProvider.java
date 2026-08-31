package za.co.techaim.target;

// A minimal content provider for sharing Tech Aim files off the tablet.
//
// WHY NOT androidx FileProvider. It would work, but it drags in the androidx
// core dependency, which cannot be satisfied by the vendored-AAR approach the
// rest of this project uses (fileTree over android/libs) and would force
// either a network-resolved Gradle dependency or hand-editing the build.gradle
// androiddeployqt generates. Both are more fragile than the sixty lines below,
// and both would be a permanent maintenance cost for one Intent.
//
// WHAT IT EXPOSES. Read-only, and ONLY files inside the application's private
// files directory. Nothing else is reachable: a path that escapes that root is
// refused, so a malformed or hostile request cannot walk out of the sandbox.
//
// Sharing still uses a content:// URI with a one-shot read grant, exactly as a
// file:// URI is forbidden to.

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

public class TechAimFileProvider extends ContentProvider
{
    @Override public boolean onCreate() { return true; }

    // Resolves a URI to a file, refusing anything outside the private files
    // directory. getCanonicalPath() is what defeats "../" traversal: comparing
    // the raw path would let a crafted URI climb out.
    private File resolve(Uri uri) throws FileNotFoundException
    {
        try {
            File root = getContext().getFilesDir().getCanonicalFile();
            String path = uri.getPath();
            if (path == null) throw new FileNotFoundException("no path");
            if (path.startsWith("/")) path = path.substring(1);
            File f = new File(root, path).getCanonicalFile();
            if (!f.getPath().startsWith(root.getPath()))
                throw new FileNotFoundException("outside the app files root");
            if (!f.exists())
                throw new FileNotFoundException(f.getPath());
            return f;
        } catch (IOException e) {
            throw new FileNotFoundException("cannot resolve: " + e);
        }
    }

    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode)
        throws FileNotFoundException
    {
        // READ ONLY. A share must never become a write channel into the app's
        // own data.
        return ParcelFileDescriptor.open(resolve(uri),
                                         ParcelFileDescriptor.MODE_READ_ONLY);
    }

    // Mail and messaging apps ask for the display name and size before they
    // will attach anything; without this the attachment shows as "unknown" or
    // is silently dropped.
    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
                        String[] selectionArgs, String sortOrder)
    {
        try {
            File f = resolve(uri);
            MatrixCursor c = new MatrixCursor(
                new String[]{ OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE });
            c.addRow(new Object[]{ f.getName(), f.length() });
            return c;
        } catch (FileNotFoundException e) {
            return null;
        }
    }

    @Override public String getType(Uri uri) { return "application/octet-stream"; }

    // Deliberately unsupported: this provider exists to hand out one readable
    // file, not to be a general data store.
    @Override public Uri insert(Uri uri, ContentValues values) { return null; }
    @Override public int delete(Uri uri, String s, String[] a) { return 0; }
    @Override public int update(Uri uri, ContentValues v, String s, String[] a) { return 0; }
}

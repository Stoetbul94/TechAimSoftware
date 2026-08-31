package za.co.techaim.target;

// Tech Aim — Android file sharing.
//
// WHY THIS EXISTS. On Windows an operator emails a zip off the Desktop. On
// Android there is no Desktop and no file manager path an ordinary customer
// will find, so without this the only way to get a support bundle off a tablet
// is adb - a developer tool, at a range, from someone holding a rifle.
//
// A raw file:// URI is not an option: handing one to another app throws
// FileUriExposedException on API 24 and above. Everything shared here goes out
// as a content:// URI from the FileProvider declared in the manifest, with a
// one-shot read grant attached to the Intent.
//
// ONE sharing mechanism, used by both the support bundle and report/PDF
// export, so there is no second half-working path to maintain.

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;

import java.io.File;

public class ShareBridge
{
    private static final String TAG = "TechAimShare";
    private static final String AUTHORITY = "za.co.techaim.target.fileprovider";

    private static Activity s_activity;

    public static void setActivity(Activity activity) { s_activity = activity; }

    // Shares a single file. Returns false rather than throwing, so a failed
    // share degrades to a message the operator can read instead of a crash
    // report they cannot.
    public static boolean shareFile(String absolutePath, String mimeType,
                                    String title)
    {
        try {
            if (s_activity == null) { Log.w(TAG, "no activity"); return false; }
            File f = new File(absolutePath);
            if (!f.exists()) { Log.w(TAG, "missing: " + absolutePath); return false; }

            // The path is made RELATIVE to the private files dir, because
            // that is the only root TechAimFileProvider will resolve.
            String root = s_activity.getFilesDir().getCanonicalPath();
            String abs  = f.getCanonicalPath();
            if (!abs.startsWith(root)) {
                Log.w(TAG, "refusing to share outside the app files root");
                return false;
            }
            String rel = abs.substring(root.length()).replace(File.separatorChar, '/');
            if (rel.startsWith("/")) rel = rel.substring(1);
            Uri uri = new Uri.Builder().scheme("content")
                          .authority(AUTHORITY).appendEncodedPath(rel).build();

            Intent send = new Intent(Intent.ACTION_SEND);
            send.setType(mimeType == null || mimeType.isEmpty()
                         ? "application/octet-stream" : mimeType);
            send.putExtra(Intent.EXTRA_STREAM, uri);
            send.putExtra(Intent.EXTRA_SUBJECT, title);
            // The grant travels WITH the intent and expires with it. No
            // permanent permission is given to any app.
            send.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

            Intent chooser = Intent.createChooser(send, title);
            chooser.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            s_activity.startActivity(chooser);
            return true;
        } catch (Throwable t) {
            Log.w(TAG, "share failed: " + t);
            return false;
        }
    }

    // A support bundle is a DIRECTORY (QtCore has no archiver - see
    // SupportBundle.h). Android cannot share a directory, so the caller zips
    // it first and shares the zip; this method exists so the mime type for
    // that case lives in one place.
    public static boolean shareZip(String absolutePath, String title)
    {
        return shareFile(absolutePath, "application/zip", title);
    }

    public static boolean sharePdf(String absolutePath, String title)
    {
        return shareFile(absolutePath, "application/pdf", title);
    }
}

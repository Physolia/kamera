
#ifndef __kamera_h__
#define __kamera_h__

#include <kio/slavebase.h>

extern "C" {
	#include <gphoto2.h>
// Defination for this function in gphoto2.h is missing
	int gp_camera_exit(Camera *camera);
}

class KameraProtocol : public KIO::SlaveBase
{
public:
	KameraProtocol(const QCString &pool, const QCString &app);
	virtual ~KameraProtocol();

	virtual void get(const KURL &url);
	virtual void stat(const KURL &url);
	virtual void del(const KURL &url, bool isFile);
	void listDir(const KURL &url);

private:
	Camera *m_camera;
	bool m_previewThumbs;

	void loadSettings(void);
	bool openCamera(void);
	void closeCamera(void);

	void statRoot(void);
	void statRegular(const KURL &url);
	bool stripCachePath(KURL &url);
	void translateCLEToUDS(KIO::UDSEntry &udsEntry,
				const CameraListEntry &cleEntry);
	bool cameraSupportsPreview(void);
	bool cameraSupportsDel(void);
	bool cameraSupportsPut(void);
	bool findCameraListEntry(const KURL &url, CameraListEntry &cle);
	int readCameraFolder(CameraList *list, const QString &folder);
};

#endif

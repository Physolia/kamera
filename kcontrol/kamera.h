#ifndef __kamera_h__
#define __kamera_h__

#include <kcmodule.h>

extern "C"
{
	#include <gphoto2.h>
}

class QWidget;
class QRadioButton;
class QPushButton;
class QComboBox;
class QVButtonGroup;
class QLineEdit;
class QWidgetStack;
class QCheckBox;
class QIconViewItem;

class KCamera;
class KameraDeviceSelectDialog;
class KSimpleConfig;
class KIconView;
class KActionCollection;
class KToolBar;

class KKameraConfig : public KCModule
{
	Q_OBJECT
	friend class KameraDeviceSelectDialog;

public:
	KKameraConfig(QWidget *parent, const char *name, const QStringList &);
	virtual ~KKameraConfig();

	// KCModule interface methods
	void load();
	void save();
	void defaults();
	int buttons();
	QString quickHelp() const;

protected:
	QString suggestName(const QString &name);

protected slots:
	void slot_deviceMenu(QIconViewItem *item, const QPoint &point);
	void slot_deviceSelected(QIconViewItem *item);
	void slot_addCamera();
	void slot_removeCamera();
	void slot_configureCamera();
	void slot_cameraSummary();
	void slot_testCamera();
	void slot_error(const QString &message);
	void slot_error(const QString &message, const QString &details);

private:
	KSimpleConfig *m_config;
	typedef QMap<QString, KCamera *> CameraDevicesMap;
	CameraDevicesMap m_devices;

	// manage widgets
	void displayGPFailureDialogue(void);
	void displayGPSuccessDialogue(void);
	void displayCameraAbilities(const CameraAbilities &abilities);
	void populateDeviceListView(void);

	void autoDetect(void);
	
	// camera device selection listview
	KIconView *m_deviceSel;
	KActionCollection *m_actions;
	QPushButton *m_addCamera, *m_removeCamera, *m_testCamera, *m_configureCamera;
	KToolBar *m_toolbar;
	QPopupMenu *m_devicePopup;

	// true if libgphoto2 was initialised successfully in
	// the constructor
	bool m_gpInitialised;

	static KKameraConfig *m_instance;
};

#endif


#include <qlabel.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <qvbuttongroup.h>
#include <qgrid.h>
#include <qwidgetstack.h>
#include <qcheckbox.h>
#include <qdir.h>
#include <qwhatsthis.h>
#include <qregexp.h>
#include <qpopupmenu.h>

#include <kgenericfactory.h>
#include <ksimpleconfig.h>
#include <kaction.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kiconview.h>
#include <kdialog.h>
#include <klocale.h>
#include <kglobal.h>
#include <ktoolbar.h>

#include <kprotocolinfo.h>
#include <kdebug.h>

#include "kameraconfigdialog.h"
#include "kameradevice.h"
#include "kamera.h"
#include "kamera.moc"

// XXX HACK HACK HACK
// XXX All tocstr(string) references can be safely replaced with
// XXX string.latin1() as soon as the gphoto2 API uses 'const char *'
// XXX instead of 'char *' in calls that don't modify the string
#define tocstr(x) ((char *)((x).latin1()))

typedef KGenericFactory<KKameraConfig, QWidget> KKameraConfigFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_kamera, KKameraConfigFactory( "kcmkamera" ) );

// --------------- Camera control center module widget ---

KKameraConfig *KKameraConfig::m_instance = NULL;

KKameraConfig::KKameraConfig(QWidget *parent, const char *name, const QStringList &)
	: KCModule(KKameraConfigFactory::instance(), parent, name)
{
	m_devicePopup = new QPopupMenu(this);
	m_actions = new KActionCollection(this);
	m_config = new KSimpleConfig(KProtocolInfo::config("camera"));

	// store instance for frontend_prompt
	m_instance = this;

	// build and display normal dialogue
	displayGPSuccessDialogue();

	// autodetect new cameras
	autoDetect();

	// load existing configuration
	load();
}

KKameraConfig::~KKameraConfig()
{
}

void KKameraConfig::defaults()
{
}

void KKameraConfig::displayGPFailureDialogue(void)
{
	new QLabel(i18n("Unable to initialize the gPhoto2 libraries."), this);
}

void KKameraConfig::displayGPSuccessDialogue(void)
{
	// create a layout with two vertical boxes
	QVBoxLayout *topLayout = new QVBoxLayout(this, KDialog::marginHint(), KDialog::spacingHint());
	topLayout->setAutoAdd(true);
	
	m_toolbar = new KToolBar(this, "ToolBar");
	
	// create list of devices
	m_deviceSel = new KIconView(this);
	connect(m_deviceSel, SIGNAL(rightButtonClicked(QIconViewItem *, const QPoint &)), SLOT(slot_deviceMenu(QIconViewItem *, const QPoint &)));
	connect(m_deviceSel, SIGNAL(selectionChanged(QIconViewItem *)), SLOT(slot_deviceSelected(QIconViewItem *)));
	m_deviceSel->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	
	// create actions
	KAction *act;
	
	act = new KAction(i18n("Add"), "camera", 0, this, SLOT(slot_addCamera()), m_actions, "camera_add");
	act->setWhatsThis(i18n("Click this button to add a new camera."));
	act->plug(m_toolbar);
	m_toolbar->insertLineSeparator();
	act = new KAction(i18n("Test"), "camera_test", 0, this, SLOT(slot_testCamera()), m_actions, "camera_test");
	act->setWhatsThis(i18n("Click this button to remove the selected camera from the list."));
	act->plug(m_toolbar);
	act = new KAction(i18n("Remove"), "edittrash", 0, this, SLOT(slot_removeCamera()), m_actions, "camera_remove");
	act->setWhatsThis(i18n("Click this button to remove the selected camera from the list."));
	act->plug(m_toolbar);
	act = new KAction(i18n("Configure"), "configure", 0, this, SLOT(slot_configureCamera()), m_actions, "camera_configure");
	act->setWhatsThis(i18n("Click this button to change the configuration of the selected camera.<br><br>The availability of this feature and the contents of the Configuration dialog depend on the camera model."));
	act->plug(m_toolbar);

	act = new KAction(i18n("Information"), "hwinfo", 0, this, SLOT(slot_cameraSummary()), m_actions, "camera_summary");
	act->setWhatsThis(i18n("Click this button to view a summary of the current status of the selected camera.<br><br>The availability of this feature and the contents of the Configuration dialog depend on the camera model."));
	act->plug(m_toolbar);
}

void KKameraConfig::populateDeviceListView(void)
{
	m_deviceSel->clear();
	CameraDevicesMap::Iterator it;
	for (it = m_devices.begin(); it != m_devices.end(); it++) {
		if (it.data()) {
			new QIconViewItem(m_deviceSel, it.key(), KGlobal::iconLoader()->loadIcon("camera", KIcon::Desktop));
		}
	}
	slot_deviceSelected(m_deviceSel->currentItem());
}

void KKameraConfig::save(void)
{
	CameraDevicesMap::Iterator it;

	for (it = m_devices.begin(); it != m_devices.end(); it++)
	{
		it.data()->save(m_config);
	}
	m_config->sync();
}

void KKameraConfig::autoDetect(void)
{
	GPContext *glob_context = NULL;
	QStringList groupList = m_config->groupList();

        int i, count;
        CameraList list;
        CameraAbilitiesList *al;
        GPPortInfoList *il;
        const char *model, *value;

        gp_abilities_list_new (&al);
        gp_abilities_list_load (al, glob_context);
        gp_port_info_list_new (&il);
        gp_port_info_list_load (il);
        gp_abilities_list_detect (al, il, &list, glob_context);
        gp_abilities_list_free (al);
        gp_port_info_list_free (il);

        count = gp_list_count (&list);
	
	for (i = 0 ; i<count ; i++) {
		gp_list_get_name  (&list, i, &model);
		gp_list_get_value (&list, i, &value);

		if (groupList.contains(model))
			continue;
		kdDebug() << "Adding " << model << " at " << value << endl;
		m_config->setGroup(model);
		m_config->writeEntry("Model",model);
		m_config->writeEntry("Path",value);
	}
}

void KKameraConfig::load(void)
{
	QStringList groupList = m_config->groupList();
	QStringList::Iterator it;
	
	for (it = groupList.begin(); it != groupList.end(); it++) {
		if (*it != "<default>")	{
			KCamera *kcamera = new KCamera(*it);
			connect(kcamera, SIGNAL(error(const QString &)), SLOT(slot_error(const QString &)));
			connect(kcamera, SIGNAL(error(const QString &, const QString &)), SLOT(slot_error(const QString &, const QString &)));
			kcamera->load(m_config);
			m_devices[*it] = kcamera;
		}
	}
	
	populateDeviceListView();
}

QString KKameraConfig::suggestName(const QString &name)
{
	QString new_name = name;
	new_name.replace(QRegExp("/"), ""); // we cannot have a slash in a URI's host

	if (!m_devices.contains(new_name)) return new_name;
	
	// try new names with a number appended until we find a free one
	int i = 1;
	while (i++ < 0xffff) {
		new_name = name + " (" + QString::number(i) + ")";
		if (!m_devices.contains(new_name)) return new_name;
	}

	return QString::null;
}

void KKameraConfig::slot_addCamera()
{
	KCamera *m_device = new KCamera(QString::null);
	connect(m_device, SIGNAL(error(const QString &)), SLOT(slot_error(const QString &)));
	connect(m_device, SIGNAL(error(const QString &, const QString &)), SLOT(slot_error(const QString &, const QString &)));
	KameraDeviceSelectDialog dialog(this, m_device);
	if (dialog.exec() == QDialog::Accepted) {
		dialog.save();
		m_device->setName(suggestName(m_device->model()));
		m_devices.insert(m_device->name(), m_device);
		populateDeviceListView();
	} else {
		delete m_device;
	}
}

void KKameraConfig::slot_removeCamera()
{
	QString name = m_deviceSel->currentItem()->text();
	if (m_devices.contains(name)) {
		KCamera *m_device = m_devices[name];
		m_devices.remove(name);
		delete m_device;
		m_config->deleteGroup(name, true);
		populateDeviceListView();
	}
}

void KKameraConfig::slot_testCamera()
{
	QString name = m_deviceSel->currentItem()->text();
	if (m_devices.contains(name)) {
		KCamera *m_device = m_devices[name];
		if (m_device->test())
			KMessageBox::information(this, i18n("Camera test was successful."));
	}
}

void KKameraConfig::slot_configureCamera()
{
	QString name = m_deviceSel->currentItem()->text();
	if (m_devices.contains(name)) {
		KCamera *m_device = m_devices[name];
		m_device->configure();
	}
}

void KKameraConfig::slot_cameraSummary()
{
	QString summary;
	QString name = m_deviceSel->currentItem()->text();
	if (m_devices.contains(name)) {
		KCamera *m_device = m_devices[name];
		summary = m_device->summary();
		if (QString::null != summary) {
			KMessageBox::information(this, summary);
		}
	}
}

void KKameraConfig::slot_deviceMenu(QIconViewItem *item, const QPoint &point)
{
	if (item) {
		QString name = item->text();
		m_devicePopup->clear();
		m_actions->action("camera_test")->plug(m_devicePopup);
		m_actions->action("camera_remove")->plug(m_devicePopup);
		m_actions->action("camera_configure")->plug(m_devicePopup);
		m_actions->action("camera_summary")->plug(m_devicePopup);
		m_devicePopup->popup(point);
	}
}

void KKameraConfig::slot_deviceSelected(QIconViewItem *item)
{
	m_actions->action("camera_test")->setEnabled(item);
	m_actions->action("camera_remove")->setEnabled(item);
	m_actions->action("camera_configure")->setEnabled(item);
	m_actions->action("camera_summary")->setEnabled(item);
}

QString KKameraConfig::quickHelp() const
{
	return i18n("<h1>Kamera Configuration</h1>\n"
	  "This module allows you to configure support for your digital camera.\n"
	  "You would need to select the camera's model and the port it is connected\n"
	  "to on your computer (e.g. USB, Serial, Firewire). If your camera doesn't\n"
	  "appear in the list of <i>Supported Cameras</i>, go to the\n"
	  "<a href=\"http://www.gphoto.org\">GPhoto web site</a> for a possible update.<br><br>\n"
	  "To view and download images from the digital camera, go to address\n"
	  "<a href=\"camera:/\">camera:/</a> in Konqueror and other KDE applications.");
}

void KKameraConfig::slot_error(const QString &message)
{
	KMessageBox::error(this, message);
}

void KKameraConfig::slot_error(const QString &message, const QString &details)
{
	KMessageBox::detailedError(this, message, details);
}

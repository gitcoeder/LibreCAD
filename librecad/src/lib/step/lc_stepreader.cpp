#include "lc_stepreader.h"

#ifdef LC_HAVE_OCCT

#include <QFileInfo>
#include <QObject>

#include <IFSelect_ReturnStatus.hxx>
#include <Interface_Static.hxx>
#include <STEPControl_Reader.hxx>

LC_StepReader::Result LC_StepReader::read(const QString& fileName) {
    Result result;

    if (!QFileInfo::exists(fileName)) {
        result.errorMessage = QObject::tr("File does not exist: %1", "LC_StepReader").arg(fileName);
        return result;
    }

    // STEPControl_Reader's constructor registers the xstep.* Interface_Static
    // parameters (via STEPControl_Controller::Init()); SetCVal must run after
    // that registration or it silently targets an undefined parameter.
    STEPControl_Reader reader;

    // Normalize the working unit to millimeters regardless of what the STEP
    // file itself declares (inch, meter, ...) -- OCCT's STEP translator
    // performs the scaling for us during transfer.
    Interface_Static::SetCVal("xstep.cascade.unit", "MM");

    const QByteArray pathUtf8 = fileName.toUtf8();
    const IFSelect_ReturnStatus status = reader.ReadFile(pathUtf8.constData());
    if (status != IFSelect_RetDone) {
        result.errorMessage = QObject::tr(
            "Could not parse STEP file (invalid or unsupported STEP data): %1",
            "LC_StepReader").arg(fileName);
        return result;
    }

    result.rootCount = reader.NbRootsForTransfer();
    if (result.rootCount <= 0) {
        result.errorMessage = QObject::tr(
            "STEP file contains no transferable roots (no shape representation found): %1",
            "LC_StepReader").arg(fileName);
        return result;
    }

    const Standard_Integer transferredCount = reader.TransferRoots();
    if (transferredCount <= 0) {
        result.errorMessage = QObject::tr(
            "STEP file was read but no geometry could be transferred: %1",
            "LC_StepReader").arg(fileName);
        return result;
    }

    result.shape = reader.OneShape();
    if (result.shape.IsNull()) {
        result.errorMessage = QObject::tr(
            "STEP file transfer produced no usable shape: %1",
            "LC_StepReader").arg(fileName);
        return result;
    }

    result.success = true;
    return result;
}

#endif // LC_HAVE_OCCT

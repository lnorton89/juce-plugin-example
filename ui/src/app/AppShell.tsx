import { useCallback, useState } from 'react';
import Box from '@mui/material/Box';
import { useBridgeRequest, useBridgeStatus, useLicenseRequest, useLicenseStatus } from '../bridge/BridgeProvider';
import type { LicensingStateEnum } from '../bridge/protocol';
import { AnalyzerStage } from '../components/AnalyzerStage';
import { ActivationDialog } from '../components/ActivationDialog';
import { BrandHeader } from '../components/BrandHeader';
import { DeactivationDialog } from '../components/DeactivationDialog';
import { GraceWarnAlert } from '../components/GraceWarnAlert';
import { StandaloneSourceStrip } from '../components/StandaloneSourceStrip';
import { StatusFooter } from '../components/StatusFooter';

type DialogState = 'none' | 'activation' | 'deactivation';

export function AppShell() {
  const bridge = useBridgeStatus();
  const licenseStatus = useLicenseStatus();
  const request = useBridgeRequest();
  const licenseRequest = useLicenseRequest();
  const mode = bridge.state === 'ready' ? bridge.hostInfo.hostMode : undefined;
  const retryConnection = useCallback(() => window.location.reload(), []);
  const [dialog, setDialog] = useState<DialogState>('none');
  const [activating, setActivating] = useState(false);
  const [activationErrorCode, setActivationErrorCode] = useState<string | undefined>();
  const [activationErrorMessage, setActivationErrorMessage] = useState<string | undefined>();
  const [activationSuccess, setActivationSuccess] = useState(false);
  const [deactivating, setDeactivating] = useState(false);
  const [deactivationError, setDeactivationError] = useState<string | undefined>();
  const [deactivationSuccess, setDeactivationSuccess] = useState(false);

  const handleSourceSelect = useCallback((mode: string, sourceId: string) => {
    request?.sendSourceSelect(mode, sourceId);
  }, [request]);

  const handleSourceStop = useCallback(() => {
    request?.sendSourceStop();
  }, [request]);

  const handleLicenseClick = useCallback(() => {
    if (!licenseStatus) return;
    const state = licenseStatus.state;
    if (state === 'activating' || state === 'deactivating') return;
    if (state === 'not_activated' || state === 'revalidation_required' || state === 'revoked' || state === 'corrupt') {
      setActivating(false);
      setActivationErrorCode(undefined);
      setActivationErrorMessage(undefined);
      setActivationSuccess(false);
      setDialog('activation');
    } else if (state === 'activated' || state === 'offline_grace') {
      setDeactivating(false);
      setDeactivationError(undefined);
      setDeactivationSuccess(false);
      setDialog('deactivation');
    }
  }, [licenseStatus]);

  const handleActivationClose = useCallback(() => {
    setDialog('none');
    setActivating(false);
  }, []);

  const handleActivate = useCallback((licenseKey: string) => {
    if (!licenseRequest) return;
    setActivating(true);
    setActivationErrorCode(undefined);
    setActivationErrorMessage(undefined);
    setActivationSuccess(false);
    licenseRequest.sendActivate(licenseKey);
  }, [licenseRequest]);

  const handleDeactivationClose = useCallback(() => {
    setDialog('none');
    setDeactivating(false);
  }, []);

  const handleDeactivate = useCallback(() => {
    if (!licenseRequest) return;
    setDeactivating(true);
    setDeactivationError(undefined);
    setDeactivationSuccess(false);
    licenseRequest.sendDeactivate();
  }, [licenseRequest]);

  // Watch for license status changes to update dialog state
  if (licenseStatus && (dialog === 'activation' || dialog === 'deactivation')) {
    if (activating && licenseStatus.state === 'activated') {
      setActivating(false);
      setActivationSuccess(true);
    } else if (activating && (licenseStatus.state === 'not_activated' || licenseStatus.state === 'revalidation_required' || licenseStatus.state === 'revoked' || licenseStatus.state === 'corrupt') && licenseStatus.code) {
      setActivating(false);
      setActivationErrorCode(licenseStatus.code);
      setActivationErrorMessage(licenseStatus.message);
    } else if (activating && (licenseStatus.state === 'service_unavailable') && licenseStatus.code) {
      setActivating(false);
      setActivationErrorCode(licenseStatus.code);
      setActivationErrorMessage(licenseStatus.message);
    } else if (deactivating && licenseStatus.state === 'not_activated') {
      setDeactivating(false);
      setDeactivationSuccess(true);
    } else if (deactivating && (licenseStatus.state === 'activated' || licenseStatus.state === 'offline_grace') && licenseStatus.code) {
      setDeactivating(false);
      setDeactivationError(licenseStatus.message);
    }
  }

  return (
    <Box
      sx={{
        width: '100vw',
        height: '100vh',
        minWidth: 0,
        display: 'grid',
        gridTemplateRows: '52px auto minmax(0, 1fr) 32px',
        bgcolor: 'background.default',
        color: 'text.primary',
        overflow: 'hidden',
      }}
    >
      <BrandHeader mode={mode} />
      <StandaloneSourceStrip
        bridge={bridge}
        onSourceSelect={handleSourceSelect}
        onSourceStop={handleSourceStop}
      />
      {licenseStatus && licenseStatus.offlineGraceRemainingDays >= 0 && licenseStatus.offlineGraceRemainingDays < 3 && (
        <GraceWarnAlert offlineGraceRemainingDays={licenseStatus.offlineGraceRemainingDays} />
      )}
      <Box
        component="main"
        sx={{ minWidth: 0, minHeight: 0, p: 4, '@media (max-width:639px)': { p: 3 } }}
      >
        <AnalyzerStage bridge={bridge} onRetry={retryConnection} />
      </Box>
      <StatusFooter bridge={bridge} licensing={licenseStatus} onLicenseClick={handleLicenseClick} />

      <ActivationDialog
        open={dialog === 'activation'}
        onClose={handleActivationClose}
        onActivate={handleActivate}
        activating={activating}
        errorCode={activationErrorCode}
        errorMessage={activationErrorMessage}
        success={activationSuccess}
      />

      <DeactivationDialog
        open={dialog === 'deactivation'}
        onClose={handleDeactivationClose}
        onDeactivate={handleDeactivate}
        deactivating={deactivating}
        errorMessage={deactivationError}
        success={deactivationSuccess}
      />
    </Box>
  );
}

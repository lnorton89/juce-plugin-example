import VpnKeyOutlined from '@mui/icons-material/VpnKeyOutlined';
import Alert from '@mui/material/Alert';
import Button from '@mui/material/Button';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogTitle from '@mui/material/DialogTitle';
import LinearProgress from '@mui/material/LinearProgress';
import TextField from '@mui/material/TextField';
import { useCallback, useState } from 'react';
import { projectConfig } from '../config/projectConfig';

export interface ActivationDialogProps {
  open: boolean;
  onClose: () => void;
  onActivate: (licenseKey: string) => void;
  activating: boolean;
  errorCode?: string;
  errorMessage?: string;
  success: boolean;
}

const errorSeverity: Record<string, 'warning' | 'error'> = {
  license_already_activated: 'warning',
  machine_count_exceeded: 'warning',
  rate_limited: 'warning',
  maintenance_mode: 'warning',
  network_error: 'warning',
  dns_lookup_failed: 'warning',
  tls_error: 'warning',
  timeout: 'warning',
};

function userMessage(code: string, fallback: string): string {
  const messages: Record<string, string> = {
    invalid_license_key: 'The license key you entered is not valid. Please check and try again.',
    license_already_activated: 'This license is already activated on another machine. Deactivate that machine first, or contact support.',
    license_expired: 'This license has expired.',
    license_revoked: 'This license has been revoked.',
    activation_not_found: 'No activation found for this license. Please activate again.',
    machine_count_exceeded: 'This license is at its activation limit. Deactivate another machine first.',
    rate_limited: 'Too many requests. Please wait a moment and try again.',
    internal_error: 'The activation service encountered an error. Please try again later.',
    maintenance_mode: 'Activation service is undergoing maintenance. Please try again later.',
    network_error: 'Unable to reach the activation service. Check your internet connection and try again.',
    dns_lookup_failed: 'Activation service could not be found. Check your internet connection.',
    tls_error: 'Secure connection to the activation service failed. Your network may be blocking it.',
    timeout: 'The activation service did not respond in time. Please try again.',
  };
  return messages[code] || fallback;
}

export function ActivationDialog({ open, onClose, onActivate, activating, errorCode, errorMessage, success }: ActivationDialogProps) {
  const [keyInput, setKeyInput] = useState('');
  const [hasInteracted, setHasInteracted] = useState(false);

  const handleActivate = useCallback(() => {
    if (!keyInput.trim()) return;
    setHasInteracted(true);
    onActivate(keyInput.trim());
  }, [keyInput, onActivate]);

  const handleKeyDown = useCallback((e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !activating && !success) {
      handleActivate();
    }
  }, [activating, success, handleActivate]);

  const handleClose = useCallback(() => {
    if (activating) return;
    setKeyInput('');
    setHasInteracted(false);
    onClose();
  }, [activating, onClose]);

  const handleSuccessClose = useCallback(() => {
    setKeyInput('');
    setHasInteracted(false);
    onClose();
  }, [onClose]);

  const showError = hasInteracted && errorCode != null && !activating && !success;
  const showSuccess = success && !activating;

  const severity = errorCode ? (errorSeverity[errorCode] ?? 'error') : 'error';
  const displayMessage = errorCode ? userMessage(errorCode, errorMessage ?? 'An unexpected error occurred. Code: ' + errorCode + '. Please contact support if this persists.') : '';

  return (
    <Dialog fullWidth maxWidth="sm" open={open} onClose={handleClose} aria-labelledby="activation-dialog-title">
      <DialogTitle id="activation-dialog-title" variant="h2">
        {showSuccess ? 'Activation complete' : activating ? 'Activating\u2026' : `Activate ${projectConfig.productName}`}
      </DialogTitle>
      <DialogContent>
        {activating && <LinearProgress sx={{ my: 1 }} aria-label="Activating" />}
        {showSuccess && (
          <Alert severity="success" sx={{ mb: 2 }}>
            {projectConfig.productName} is now activated and licensed.
          </Alert>
        )}
        {showError && (
          <Alert severity={severity} sx={{ mb: 2 }}>
            {displayMessage}
          </Alert>
        )}
        <TextField
          fullWidth
          variant="outlined"
          size="small"
          autoFocus
          disabled={activating || success}
          label="License Key"
          placeholder="XXXXXX-XXXXXX-XXXXXX-XXXXXX-XXXXXX"
          value={keyInput}
          onChange={(e) => setKeyInput(e.target.value)}
          onKeyDown={handleKeyDown}
          slotProps={{
            input: {
              startAdornment: <VpnKeyOutlined sx={{ mr: 1, color: 'text.secondary', fontSize: 20 }} />,
            },
          }}
          sx={{ mb: 1.5 }}
        />
      </DialogContent>
      <DialogActions sx={{ px: 3, pb: 2 }}>
        {showSuccess ? (
          <Button variant="contained" onClick={handleSuccessClose}>Continue</Button>
        ) : (
          <>
            <Button disabled={activating} onClick={handleClose}>Cancel</Button>
            <Button variant="contained" disabled={activating || !keyInput.trim()} onClick={handleActivate}>
              {activating ? 'Activating\u2026' : showError ? 'Try again' : 'Activate'}
            </Button>
          </>
        )}
      </DialogActions>
    </Dialog>
  );
}

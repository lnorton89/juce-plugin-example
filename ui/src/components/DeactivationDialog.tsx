import WarningAmberOutlined from '@mui/icons-material/WarningAmberOutlined';
import Alert from '@mui/material/Alert';
import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogTitle from '@mui/material/DialogTitle';
import LinearProgress from '@mui/material/LinearProgress';
import Typography from '@mui/material/Typography';

export interface DeactivationDialogProps {
  open: boolean;
  onClose: () => void;
  onDeactivate: () => void;
  deactivating: boolean;
  errorMessage?: string;
  success: boolean;
}

export function DeactivationDialog({ open, onClose, onDeactivate, deactivating, errorMessage, success }: DeactivationDialogProps) {
  const handleClose = () => {
    if (deactivating) return;
    onClose();
  };

  const handleSuccessClose = () => {
    onClose();
  };

  const showError = errorMessage != null && !deactivating && !success;

  return (
    <Dialog fullWidth maxWidth="sm" open={open} onClose={handleClose} aria-labelledby="deactivation-dialog-title">
      <DialogTitle id="deactivation-dialog-title" variant="h2">
        {deactivating ? 'Deactivating\u2026' : success ? 'Deactivation complete' : 'Deactivate this machine?'}
      </DialogTitle>
      <DialogContent>
        {deactivating && <LinearProgress sx={{ my: 1 }} aria-label="Deactivating" />}

        {success && (
          <Alert severity="success" sx={{ mb: 2 }}>
            Deactivation successful. This machine is no longer activated.
          </Alert>
        )}

        {showError && (
          <Alert severity="error" sx={{ mb: 2 }}>
            {errorMessage}
          </Alert>
        )}

        {!success && !showError && !deactivating && (
          <Box sx={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 2, py: 1 }}>
            <WarningAmberOutlined color="warning" sx={{ fontSize: 40 }} />
            <Typography variant="body2" color="text.secondary" sx={{ textAlign: 'center' }}>
              This will free your activation so you can activate LumaScope on another computer.
              You can reactivate here at any time.
            </Typography>
          </Box>
        )}
      </DialogContent>
      <DialogActions sx={{ px: 3, pb: 2 }}>
        {success ? (
          <Button variant="contained" onClick={handleSuccessClose}>Close</Button>
        ) : (
          <>
            <Button
              variant="outlined"
              disabled={deactivating}
              onClick={handleClose}
            >
              Keep activated
            </Button>
            <Button
              variant="contained"
              color="error"
              disabled={deactivating}
              onClick={onDeactivate}
            >
              {deactivating ? 'Deactivating\u2026' : 'Deactivate'}
            </Button>
          </>
        )}
      </DialogActions>
    </Dialog>
  );
}

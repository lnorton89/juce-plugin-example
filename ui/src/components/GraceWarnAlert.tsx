import Alert from '@mui/material/Alert';
import Collapse from '@mui/material/Collapse';
import { useCallback, useState } from 'react';
import { projectConfig } from '../config/projectConfig';

export interface GraceWarnAlertProps {
  offlineGraceRemainingDays: number;
}

export function GraceWarnAlert({ offlineGraceRemainingDays }: GraceWarnAlertProps) {
  const [dismissed, setDismissed] = useState(false);

  const handleDismiss = useCallback(() => {
    setDismissed(true);
  }, []);

  const showAlert = !dismissed && offlineGraceRemainingDays >= 0;
  if (!showAlert) return null;

  if (offlineGraceRemainingDays >= 3) return null;

  let severity: 'warning' | 'error';
  let message: string;

  if (offlineGraceRemainingDays <= 0) {
    severity = 'error';
    message = `Offline grace expired. Please activate to continue using ${projectConfig.productName}.`;
  } else if (offlineGraceRemainingDays === 1) {
    severity = 'error';
    message = 'Activation expires tomorrow. Connect to the internet now to avoid interruption.';
  } else {
    severity = 'warning';
    message = `Offline activation expires in ${offlineGraceRemainingDays} days. Connect to the internet to renew.`;
  }

  const role = severity === 'error' ? 'alert' as const : 'status' as const;
  const live = severity === 'error' ? 'assertive' as const : 'polite' as const;

  return (
    <Collapse in>
      <Alert
        severity={severity}
        onClose={handleDismiss}
        role={role}
        aria-live={live}
        sx={{ mx: { xs: 1.5, sm: 5 }, my: 0 }}
      >
        {message}
      </Alert>
    </Collapse>
  );
}

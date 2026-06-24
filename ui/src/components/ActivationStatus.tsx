import BlockOutlined from '@mui/icons-material/BlockOutlined';
import CloudOffOutlined from '@mui/icons-material/CloudOffOutlined';
import ErrorOutlineOutlined from '@mui/icons-material/ErrorOutlineOutlined';
import HourglassEmpty from '@mui/icons-material/HourglassEmpty';
import LockOpenOutlined from '@mui/icons-material/LockOpenOutlined';
import VerifiedOutlined from '@mui/icons-material/VerifiedOutlined';
import WarningAmberOutlined from '@mui/icons-material/WarningAmberOutlined';
import WifiOffOutlined from '@mui/icons-material/WifiOffOutlined';
import Chip from '@mui/material/Chip';
import Tooltip from '@mui/material/Tooltip';
import type { LicensingStateEnum } from '../bridge/protocol';

export interface ActivationStatusProps {
  state: LicensingStateEnum;
  onClick: () => void;
  offlineGraceRemainingDays?: number;
}

function stateConfig(state: LicensingStateEnum, offlineGraceRemainingDays?: number) {
  switch (state) {
    case 'not_activated':
      return { color: 'default' as const, Icon: LockOpenOutlined, label: 'Activate', tooltip: 'No license activated. Click to enter your license key.' };
    case 'activating':
      return { color: 'info' as const, Icon: HourglassEmpty, label: 'Activating\u2026', tooltip: '' };
    case 'activated':
      return { color: 'success' as const, Icon: VerifiedOutlined, label: 'Activated', tooltip: 'Licensed and activated. Click to deactivate.' };
    case 'offline_grace': {
      const days = offlineGraceRemainingDays ?? 0;
      return { color: 'warning' as const, Icon: WifiOffOutlined, label: `Offline (${days}d)`, tooltip: `Operating offline. ${days} days remaining before re-activation required.` };
    }
    case 'revalidation_required':
      return { color: 'error' as const, Icon: ErrorOutlineOutlined, label: 'Re-activate', tooltip: 'The offline grace period has expired. Connect to the internet to re-activate.' };
    case 'revoked':
      return { color: 'error' as const, Icon: BlockOutlined, label: 'License revoked', tooltip: 'This license has been revoked by the issuer.' };
    case 'corrupt':
      return { color: 'error' as const, Icon: WarningAmberOutlined, label: 'License error', tooltip: 'License file could not be verified. Please activate again.' };
    case 'service_unavailable':
      return { color: 'warning' as const, Icon: CloudOffOutlined, label: 'Service offline', tooltip: 'Activation service is unreachable. Offline use continues.' };
    case 'deactivating':
      return { color: 'info' as const, Icon: HourglassEmpty, label: 'Deactivating\u2026', tooltip: '' };
  }
}

export function ActivationStatus({ state, onClick, offlineGraceRemainingDays }: ActivationStatusProps) {
  const { color, Icon, label, tooltip } = stateConfig(state, offlineGraceRemainingDays);
  const isTransitionState = state === 'activating' || state === 'deactivating';

  const chip = (
    <Chip
      size="small"
      variant="outlined"
      color={color}
      icon={<Icon sx={{ fontSize: 16 }} />}
      label={label}
      onClick={isTransitionState ? undefined : onClick}
      disabled={isTransitionState}
      aria-live="polite"
      role="status"
      sx={{
        minWidth: 120,
        height: 24,
        '& .MuiChip-label': { fontSize: 12, fontWeight: 600, px: 1 },
        '& .MuiChip-icon': { fontSize: 16, ml: 0.5 },
      }}
    />
  );

  if (isTransitionState || !tooltip) return chip;

  return (
    <Tooltip title={tooltip} placement="top" enterDelay={500}>
      {chip}
    </Tooltip>
  );
}

import { useI18n } from '../../i18n';
import styles from './LoadingScreen.module.css';

type LoadingPhase = 'downloading' | 'decoding' | 'done';

interface LoadingScreenProps {
  phase: LoadingPhase;
  downloadProgress: number;
  decodeProgress: number;
  totalSamples: number;
  totalLayers: number;
}

export function LoadingScreen({
  phase,
  downloadProgress,
  decodeProgress,
  totalSamples,
  totalLayers,
}: LoadingScreenProps) {
  const { t } = useI18n();

  const isDownloading = phase === 'downloading';
  const progress = isDownloading ? downloadProgress : decodeProgress;
  const total = isDownloading ? totalSamples : totalLayers;
  const percentage = total > 0 ? Math.round((progress / total) * 100) : 0;

  return (
    <div className={styles.overlay}>
      <div className={styles.container}>
        <img src="/logo.png" alt="" className={styles.logo} />
        <div className={styles.title}>
          {isDownloading ? t('loading.downloading') : t('loading.decoding')}
        </div>
        <div
          className={styles.progressBar}
          role="progressbar"
          aria-valuemin={0}
          aria-valuemax={100}
          aria-valuenow={percentage}
        >
          <div
            className={styles.progressFill}
            style={{ width: `${percentage}%` }}
          />
        </div>
        <div className={styles.progressText}>
          {isDownloading
            ? t('loading.samples', { progress, total })
            : t('loading.layers', { progress, total })}
        </div>
        <div className={styles.hint}>{t('loading.hint')}</div>
      </div>
    </div>
  );
}

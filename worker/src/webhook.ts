import type { LicenseStatus } from './db/schema';
import type { Env } from './env';
import { Repository } from './db/repository';

export interface LemonWebhookEvent {
  eventName: string;
  eventId: string;
  eventCreatedAt: string;
  storeId: string;
  productId: string;
  variantId: string;
  licenseKey: string | null;
  status: string;
  customerName: string | null;
  customerEmail: string | null;
}

const SUPPORTED_EVENTS = new Set([
  'order_created',
  'subscription_created',
  'subscription_updated',
  'subscription_cancelled',
  'subscription_expired',
  'license_key_created',
  'license_key_updated',
]);

const MAX_BODY_SIZE = 1_048_576; // 1 MB

function hexStringToArrayBuffer(hex: string): ArrayBuffer {
  const hexLength = hex.length;
  const bytes = new Uint8Array(hexLength / 2);
  for (let i = 0; i < hexLength; i += 2) {
    bytes[i / 2] = parseInt(hex.substring(i, i + 2), 16);
  }
  return bytes.buffer;
}

export async function verifyLemonSignature(
  request: Request,
  secret: string
): Promise<{ valid: boolean; body: string }> {
  const body = await request.text();
  const xSignature = request.headers.get('X-Signature') || request.headers.get('x-signature');

  if (!xSignature) {
    return { valid: false, body };
  }

  try {
    const encoder = new TextEncoder();
    const key = await crypto.subtle.importKey(
      'raw',
      encoder.encode(secret),
      { name: 'HMAC', hash: 'SHA-256' },
      false,
      ['verify']
    );

    const signatureBytes = hexStringToArrayBuffer(xSignature);
    const valid = await crypto.subtle.verify(
      'HMAC',
      key,
      signatureBytes,
      encoder.encode(body)
    );

    return { valid, body };
  } catch {
    return { valid: false, body };
  }
}

export function parseLemonEvent(body: string): LemonWebhookEvent | null {
  try {
    const parsed = JSON.parse(body);

    if (!parsed || typeof parsed !== 'object') return null;
    if (!parsed.meta || typeof parsed.meta !== 'object') return null;
    if (!parsed.data || typeof parsed.data !== 'object') return null;

    const meta = parsed.meta;
    const eventName = meta.event_name;
    if (typeof eventName !== 'string' || eventName.length === 0) return null;

    const customData = meta.custom_data;
    if (!customData || typeof customData !== 'object') return null;

    const data = parsed.data;
    if (typeof data.id !== 'string') return null;
    if (!data.attributes || typeof data.attributes !== 'object') return null;

    const attrs = data.attributes;

    const eventId = data.id;
    const eventCreatedAt = attrs.created_at || new Date().toISOString();
    const storeId = String(customData.store_id ?? attrs.store_id ?? '');
    const customerName = attrs.user_name ?? attrs.customer_name ?? null;
    const customerEmail = attrs.user_email ?? attrs.customer_email ?? '';

    // Extract product/variant from meta.custom_data first (merchant-configured identifiers)
    // Fall back to resource attributes for numeric IDs
    const customProductId = String(customData.product_id ?? '');
    const customVariantId = String(customData.variant_id ?? '');
    let productId = customProductId;
    let variantId = customVariantId;
    let licenseKey: string | null = null;
    let status = attrs.status ?? '';

    // If custom_data doesn't have product/variant, try resource attributes
    if (!productId && attrs.first_order_item && typeof attrs.first_order_item === 'object') {
      productId = String(attrs.first_order_item.product_id ?? '');
      variantId = String(attrs.first_order_item.variant_id ?? '');
    } else if (!productId && attrs.first_subscription_item && typeof attrs.first_subscription_item === 'object') {
      productId = String(attrs.first_subscription_item.product_id ?? '');
      variantId = String(attrs.first_subscription_item.variant_id ?? '');
    } else if (!productId && attrs.license_key && typeof attrs.license_key === 'object') {
      const lk = attrs.license_key;
      licenseKey = lk.key ?? null;
      productId = String(lk.product_id ?? '');
      variantId = String(lk.variant_id ?? '');
      status = lk.status ?? status;
    } else if (!productId) {
      productId = String(attrs.product_id ?? '');
      variantId = String(attrs.variant_id ?? '');
    }

    // Try to get license key from attributes
    if (!licenseKey) {
      licenseKey = attrs.license_key?.key ?? attrs.license_key_key ?? null;
    }

    if (!productId || !variantId) return null;

    return {
      eventName,
      eventId,
      eventCreatedAt,
      storeId,
      productId,
      variantId,
      licenseKey,
      status,
      customerName: customerName ?? null,
      customerEmail: customerEmail ?? null,
    };
  } catch {
    return null;
  }
}

function mapLemonStatusToLicenseStatus(lemonStatus: string): LicenseStatus {
  const lowerStatus = lemonStatus.toLowerCase();
  switch (lowerStatus) {
    case 'active':
    case 'paid':
      return 'active';
    case 'expired':
      return 'expired';
    case 'refunded':
      return 'refunded';
    case 'disabled':
    case 'cancelled':
      return 'disabled';
    case 'pending':
      return 'pending';
    default:
      return 'active';
  }
}

function isExpiredEvent(
  eventCreatedAt: string,
  now: Date,
  maxAgeMs: number = 300_000
): boolean {
  const eventTime = new Date(eventCreatedAt).getTime();
  if (isNaN(eventTime)) return false;
  return now.getTime() - eventTime > maxAgeMs;
}

export async function handleLemonWebhook(
  request: Request,
  env: Env,
  ctx: ExecutionContext
): Promise<Response> {
  // Content-Type check
  const contentType = request.headers.get('Content-Type') || '';
  if (!contentType.includes('application/json')) {
    return new Response(
      JSON.stringify({ error: 'Bad Request' }),
      { status: 400, headers: { 'Content-Type': 'application/json' } }
    );
  }

  // Body size check
  const contentLength = request.headers.get('Content-Length');
  if (contentLength && parseInt(contentLength, 10) > MAX_BODY_SIZE) {
    return new Response(
      JSON.stringify({ error: 'Payload Too Large' }),
      { status: 413, headers: { 'Content-Type': 'application/json' } }
    );
  }

  const repository = new Repository(env.ACTIVATION_DB);

  // Signature verification
  const { valid, body } = await verifyLemonSignature(request, env.LEMON_WEBHOOK_SECRET);
  if (!valid) {
    ctx.waitUntil(
      repository.writeAudit(
        'invalid_signature',
        'lemon_webhook',
        'webhook_event',
        undefined,
        'Signature verification failed'
      )
    );
    return new Response(
      JSON.stringify({ error: 'Unauthorized' }),
      { status: 401, headers: { 'Content-Type': 'application/json' } }
    );
  }

  // Parse event
  const event = parseLemonEvent(body);
  if (event === null) {
    ctx.waitUntil(
      repository.writeAudit(
        'parse_failure',
        'lemon_webhook',
        'webhook_event',
        undefined,
        'Failed to parse webhook event body'
      )
    );
    return new Response(
      JSON.stringify({ error: 'Bad Request' }),
      { status: 400, headers: { 'Content-Type': 'application/json' } }
    );
  }

  // Replay check: reject if event is older than 5 minutes
  if (isExpiredEvent(event.eventCreatedAt, new Date())) {
    ctx.waitUntil(
      repository.writeAudit(
        'expired_event',
        'lemon_webhook',
        'webhook_event',
        event.eventId,
        `Event ${event.eventName} is too old: ${event.eventCreatedAt}`,
        undefined,
        undefined
      )
    );
    return new Response(
      JSON.stringify({ error: 'Bad Request' }),
      { status: 400, headers: { 'Content-Type': 'application/json' } }
    );
  }

  // Idempotency check
  const existingIdempotency = await repository.findIdempotency(event.eventId);
  if (existingIdempotency !== null) {
    return new Response(
      JSON.stringify({
        status: 'processed',
        event: event.eventName,
        event_id: event.eventId,
      }),
      { status: 200, headers: { 'Content-Type': 'application/json' } }
    );
  }

  // Validate against configured Lemon identifiers
  const configuredStoreId = env.LEMON_STORE_ID || '';
  const configuredProductId = env.LEMON_PRODUCT_ID || '';
  const configuredVariantId = env.LEMON_VARIANT_ID || '';

  if (
    (configuredStoreId && event.storeId !== configuredStoreId) ||
    (configuredProductId && event.productId !== configuredProductId) ||
    (configuredVariantId && event.variantId !== configuredVariantId)
  ) {
    await repository.writeAudit(
      'unconfigured_product',
      'lemon_webhook',
      'webhook_event',
      event.eventId,
      `Skipped unconfigured store=${event.storeId} product=${event.productId} variant=${event.variantId}`
    );
    await repository.markIdempotency(
      event.eventId,
      event.eventName,
      event.eventCreatedAt,
      'skipped',
      'unconfigured product'
    );
    return new Response(
      JSON.stringify({
        status: 'skipped',
        reason: 'unconfigured product',
      }),
      { status: 200, headers: { 'Content-Type': 'application/json' } }
    );
  }

  // Check if event type is supported
  if (!SUPPORTED_EVENTS.has(event.eventName)) {
    await repository.writeAudit(
      'unsupported_event',
      'lemon_webhook',
      'webhook_event',
      event.eventId,
      `Unsupported event type: ${event.eventName}`
    );
    await repository.markIdempotency(
      event.eventId,
      event.eventName,
      event.eventCreatedAt,
      'skipped',
      'unsupported event type'
    );
    return new Response(
      JSON.stringify({
        status: 'skipped',
        reason: 'unsupported event type',
      }),
      { status: 200, headers: { 'Content-Type': 'application/json' } }
    );
  }

  // Process the event
  try {
    const licenseStatus = mapLemonStatusToLicenseStatus(event.status);

    await repository.upsertFromWebhook({
      lemon_order_id: event.eventId,
      license_key: event.licenseKey ?? event.eventId,
      product_id: event.productId,
      variant_id: event.variantId,
      store_id: event.storeId,
      status: licenseStatus,
      customer_name: event.customerName,
      customer_email: event.customerEmail,
    });

    await repository.markIdempotency(
      event.eventId,
      event.eventName,
      event.eventCreatedAt,
      'completed'
    );

    await repository.writeAudit(
      'event_processed',
      'lemon_webhook',
      'webhook_event',
      event.eventId,
      `Processed ${event.eventName} with status ${licenseStatus}`
    );

    return new Response(
      JSON.stringify({
        status: 'processed',
        event: event.eventName,
        event_id: event.eventId,
      }),
      { status: 200, headers: { 'Content-Type': 'application/json' } }
    );
  } catch (err) {
    const errorMessage = err instanceof Error ? err.message : 'Unknown error';
    await repository.markIdempotency(
      event.eventId,
      event.eventName,
      event.eventCreatedAt,
      'failed',
      errorMessage
    );
    await repository.writeAudit(
      'event_processing_failed',
      'lemon_webhook',
      'webhook_event',
      event.eventId,
      errorMessage
    );
    return new Response(
      JSON.stringify({ error: 'Internal Error' }),
      { status: 500, headers: { 'Content-Type': 'application/json' } }
    );
  }
}

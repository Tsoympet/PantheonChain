// ParthenonChain Mobile Wallet - Formatting Utilities

const ASSET_DECIMALS = {
  TALN: 8,
  DRM: 8,
  OBL: 8,
};

const ASSET_FULL_NAMES = {
  TALN: 'TALANTON',
  DRM: 'DRACHMA',
  OBL: 'OBOLOS',
};

/**
 * Format an asset amount for display.
 * @param {number} amount - The numeric amount to format
 * @param {string} asset - The asset ticker (TALN, DRM, OBL)
 * @param {number} [decimals] - Decimal places to show; defaults to 8 for known assets
 * @returns {string} Formatted amount string (e.g. "1.50000000")
 */
function formatAmount(amount, asset, decimals) {
  const places = decimals !== undefined ? decimals : (ASSET_DECIMALS[asset] !== undefined ? ASSET_DECIMALS[asset] : 8);
  return Number(amount).toFixed(places);
}

/**
 * Return the full name of an asset token.
 * @param {string} asset - The asset ticker (TALN, DRM, OBL)
 * @returns {string} Full asset name, or the ticker itself if unknown
 */
function getAssetFullName(asset) {
  return ASSET_FULL_NAMES[asset] || asset;
}

/**
 * Format a Unix timestamp as a localised date string.
 * @param {number} timestamp - Unix timestamp in seconds
 * @returns {string} Localised date string
 */
function formatDate(timestamp) {
  return new Date(timestamp * 1000).toLocaleDateString();
}

/**
 * Format a Unix timestamp as a localised date-and-time string.
 * @param {number} timestamp - Unix timestamp in seconds
 * @returns {string} Localised date-time string
 */
function formatDateTime(timestamp) {
  return new Date(timestamp * 1000).toLocaleString();
}

/**
 * Truncate a long hash or address to a compact "prefix…suffix" form.
 * @param {string} str - Full string to truncate (e.g. a txid or address)
 * @param {number} [prefixLen=8] - Characters to keep at the start
 * @param {number} [suffixLen=8] - Characters to keep at the end
 * @returns {string} Truncated string, or the original if it is short enough
 */
function truncateHash(str, prefixLen, suffixLen) {
  const pre = prefixLen !== undefined ? prefixLen : 8;
  const suf = suffixLen !== undefined ? suffixLen : 8;
  if (!str || str.length <= pre + suf + 3) {
    return str || '';
  }
  return `${str.substring(0, pre)}...${str.substring(str.length - suf)}`;
}

export { formatAmount, getAssetFullName, formatDate, formatDateTime, truncateHash };

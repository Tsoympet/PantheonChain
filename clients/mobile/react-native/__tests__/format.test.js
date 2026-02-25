// ParthenonChain Mobile Wallet - format.js Unit Tests

const formatModule = require('../src/utils/format');
const {
  formatAmount,
  getAssetFullName,
  formatDate,
  formatDateTime,
  truncateHash,
} = formatModule;

describe('formatAmount', () => {
  it('formats TALN with 8 decimal places by default', () => {
    expect(formatAmount(1.5, 'TALN')).toBe('1.50000000');
  });

  it('formats DRM with 8 decimal places by default', () => {
    expect(formatAmount(0, 'DRM')).toBe('0.00000000');
  });

  it('formats OBL with 8 decimal places by default', () => {
    expect(formatAmount(100.1, 'OBL')).toBe('100.10000000');
  });

  it('uses the provided decimals override', () => {
    expect(formatAmount(1.5, 'TALN', 2)).toBe('1.50');
  });

  it('uses 8 decimal places for an unknown asset ticker', () => {
    expect(formatAmount(1, 'XYZ')).toBe('1.00000000');
  });

  it('handles zero decimals override', () => {
    expect(formatAmount(3.7, 'TALN', 0)).toBe('4');
  });
});

describe('getAssetFullName', () => {
  it('returns TALANTON for TALN', () => {
    expect(getAssetFullName('TALN')).toBe('TALANTON');
  });

  it('returns DRACHMA for DRM', () => {
    expect(getAssetFullName('DRM')).toBe('DRACHMA');
  });

  it('returns OBOLOS for OBL', () => {
    expect(getAssetFullName('OBL')).toBe('OBOLOS');
  });

  it('returns the ticker itself for an unknown asset', () => {
    expect(getAssetFullName('XYZ')).toBe('XYZ');
  });
});

describe('formatDate', () => {
  it('returns a non-empty string for a valid timestamp', () => {
    const result = formatDate(1700000000);
    expect(typeof result).toBe('string');
    expect(result.length).toBeGreaterThan(0);
  });

  it('returns a different value for different timestamps', () => {
    const a = formatDate(1700000000); // 2023-11-14
    const b = formatDate(1700086400); // 2023-11-15
    expect(a).not.toBe(b);
  });
});

describe('formatDateTime', () => {
  it('returns a non-empty string for a valid timestamp', () => {
    const result = formatDateTime(1700000000);
    expect(typeof result).toBe('string');
    expect(result.length).toBeGreaterThan(0);
  });

  it('returns a longer or equal string compared to formatDate', () => {
    const date = formatDate(1700000000);
    const dateTime = formatDateTime(1700000000);
    expect(dateTime.length).toBeGreaterThanOrEqual(date.length);
  });
});

describe('truncateHash', () => {
  it('returns the original string when it is short enough', () => {
    const short = 'abc123';
    expect(truncateHash(short)).toBe('abc123');
  });

  it('truncates a long string with default prefix/suffix of 8', () => {
    const long = 'a'.repeat(40);
    const result = truncateHash(long);
    expect(result).toBe('aaaaaaaa...aaaaaaaa');
  });

  it('respects custom prefix and suffix lengths', () => {
    const long = '0123456789abcdef0123456789abcdef';
    const result = truncateHash(long, 4, 4);
    expect(result).toBe('0123...cdef');
  });

  it('returns empty string for falsy input', () => {
    expect(truncateHash('')).toBe('');
    expect(truncateHash(null)).toBe('');
    expect(truncateHash(undefined)).toBe('');
  });

  it('does not truncate a string exactly at the boundary', () => {
    // prefix(8) + "..." (3) + suffix(8) = 19 chars — a 19-char string should pass through unchanged
    const borderline = 'a'.repeat(19);
    expect(truncateHash(borderline)).toBe(borderline);
  });
});

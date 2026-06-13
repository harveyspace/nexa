package io.nexa;

/**
 * Result from a Nexa JS execution.
 *
 * Returned by NexaIsolate.run() / call() / load().
 * Not created directly by users — returned by Nexa engine.
 */
public class NexaResult {

    private final boolean ok;
    private final String  value;
    private final String  error;
    private final int     errorCode;

    NexaResult(boolean ok, String value, String error, int errorCode) {
        this.ok = ok;
        this.value = value;
        this.error = error;
        this.errorCode = errorCode;
    }

    public boolean   isOk()       { return ok; }
    public String    getValue()   { return value; }
    public String    getError()   { return error; }
    public int       getErrorCode(){ return errorCode; }

    @Override
    public String toString() {
        return ok ? value : "ERR[" + errorCode + "]: " + error;
    }
}

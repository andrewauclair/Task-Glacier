package dialogs;

import javax.swing.text.AttributeSet;
import javax.swing.text.BadLocationException;
import javax.swing.text.DocumentFilter;

public class TaskIDFilter extends DocumentFilter {
    // Regular expression that matches one or more digits
    private static final String DIGITS_REGEX = "\\d+";

    @Override
    public void insertString(FilterBypass fb, int offs, String str, AttributeSet a)
            throws BadLocationException {
        // Only allow insertion if the string contains only digits
        if (str != null && str.matches(DIGITS_REGEX)) {
            super.insertString(fb, offs, str, a);
        }
    }

    @Override
    public void replace(FilterBypass fb, int offs, int length, String str, AttributeSet a)
            throws BadLocationException {
        // Only allow replacement if the string contains only digits
        if (str != null && str.matches(DIGITS_REGEX)) {
            super.replace(fb, offs, length, str, a);
        }
    }
}

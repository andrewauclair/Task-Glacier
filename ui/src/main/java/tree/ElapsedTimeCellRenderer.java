package tree;

import javax.swing.*;
import javax.swing.table.TableCellRenderer;
import java.awt.*;

public class ElapsedTimeCellRenderer implements TableCellRenderer {
    @Override
    public Component getTableCellRendererComponent(JTable table, Object value, boolean isSelected, boolean hasFocus, int row, int column) {
        long minutes = (long) value;

        JLabel label = new JLabel();
        label.setOpaque(true);
        label.setHorizontalAlignment(SwingConstants.RIGHT);

        label.setForeground(isSelected ? table.getSelectionForeground() :
                table.getForeground());
        label.setBackground(isSelected ? table.getSelectionBackground() :
                table.getBackground());

        if (minutes >= 60) {
            label.setText(String.format("%02dh %02dm", minutes / 60, minutes - ((minutes / 60) * 60)));
        }
        else {
            label.setText(String.format("%02dm", minutes));
        }

        return label;
    }
}

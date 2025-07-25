package tree;

import net.byteseek.swing.treetable.TreeTableModel;

import javax.swing.table.TableColumnModel;
import javax.swing.tree.TreeNode;

public class WeeklyReportTreeTableModel extends TreeTableModel {
    public WeeklyReportTreeTableModel(TreeNode rootNode) {
        super(rootNode);
    }

    @Override
    public Object getColumnValue(TreeNode node, int column) {
        return null;
    }

    @Override
    protected TableColumnModel createTableColumnModel() {
        return null;
    }
}
